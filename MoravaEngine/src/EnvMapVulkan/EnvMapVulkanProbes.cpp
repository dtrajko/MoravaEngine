#include "EnvMapVulkanProbes.h"

#include "EnvMapVulkanProfiler.h"
#include "EnvMapVulkanShadows.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Renderer/ModelH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/Log.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <vector>


static_assert(sizeof(EnvMapVulkanProbes::ProbeVolumeUB) == 80, "std140 layout mismatch with the ProbeVolume block in Include/FrameSet.glslh");

static_assert(sizeof(EnvMapVulkanProbes::CapturePush) == 128, "layout mismatch with the Capture block in Include/ProbeCapture.glslh");

// The push constants of the update's compute passes (the ProbeUpdate block in Include/ProbeUpdate.glslh)
struct ProbeUpdatePush
{
	glm::vec4 RayRotation[3]; // the rotation's columns
	glm::ivec4 Counts;        // xyz the probe counts, w the batch's first probe
	glm::ivec4 Batch;         // x probes in the batch, y rays per probe
	glm::vec4 Params;         // hysteresis, irradiance gamma, the farthest distance the visibility stores, the environment's rotation
};
static_assert(sizeof(ProbeUpdatePush) == 96, "layout mismatch with the ProbeUpdate block in Include/ProbeUpdate.glslh");

// The CaptureSun block of the capture's fragment stage (Include/ProbeCapture_Fragment.glslh, set 1, binding 0, std140)
struct CaptureSunUB
{
	glm::mat4 ViewProjection;
	glm::vec4 Params; // x 1 when the map was drawn, y world size of a texel, z 1 / resolution, w normal bias (texels)
};
static_assert(sizeof(CaptureSunUB) == 80, "std140 layout mismatch with the CaptureSun block in Include/ProbeCapture_Fragment.glslh");

// The capture cubes' far plane (meters): what is farther counts as not seen (the sky)
static constexpr float CaptureFarPlane = 2000.0f;

// FNV-1a over the bytes of a value, continued from hash
template<typename T>
static uint64_t HashValue(uint64_t hash, const T& value)
{
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&value);
	for (size_t i = 0; i < sizeof(T); i++)
	{
		hash = (hash ^ bytes[i]) * 1099511628211ull;
	}
	return hash;
}

// The push constants of ProbeSpheres.glsl's vertex stage
struct ProbeSpheresPush
{
	glm::vec4 OriginRadius;
	glm::vec4 Spacing; // w: what the balls show (EnvMapVulkanProbeVolumeSettings::ShowMode)
	glm::ivec4 Counts;
	glm::vec4 CameraRight;
	glm::vec4 CameraUp;
};

void EnvMapVulkanProbes::Create(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	// Linear for the two atlases read per direction (the borders make it work up to a tile's edge), nearest for the
	// probes' own values
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxAnisotropy = 1.0f;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_LinearSampler));
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_NearestSampler));

	m_FillPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("ProbeEnvFill"));
	m_FillDescriptorSet = m_FillPipeline->GetShader()->CreateDescriptorSets();

	CreateSpherePipeline(sceneFramebuffer);

	// The mesh shaders' set 0 always has the atlases, also in a scene without a probe volume: the smallest ones then
	CreateAtlases(glm::ivec3(EnvMapVulkanProbeVolumeSettings::MinCount));

	CreateCaptureResources();
}

void EnvMapVulkanProbes::Destroy()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	DestroyCaptureResources();
	DestroyAtlases();
	if (m_SpherePipeline) vkDestroyPipeline(device, m_SpherePipeline, nullptr);
	if (m_SpherePipelineLayout) vkDestroyPipelineLayout(device, m_SpherePipelineLayout, nullptr);
	if (m_LinearSampler) vkDestroySampler(device, m_LinearSampler, nullptr);
	if (m_NearestSampler) vkDestroySampler(device, m_NearestSampler, nullptr);
	m_SpherePipeline = VK_NULL_HANDLE;
	m_SpherePipelineLayout = VK_NULL_HANDLE;
	m_LinearSampler = VK_NULL_HANDLE;
	m_NearestSampler = VK_NULL_HANDLE;
	m_FillPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>();
	m_FillDescriptorSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
}

void EnvMapVulkanProbes::CreateImage(Image& image, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage, uint32_t layers, bool cube)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.flags = cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = { width, height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = layers;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = usage;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK_RESULT_H2M(vkCreateImage(device, &imageInfo, nullptr, &image.Handle));

	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(device, image.Handle, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("Probes"));
	allocator.Allocate(requirements, &image.Memory);
	VK_CHECK_RESULT_H2M(vkBindImageMemory(device, image.Handle, image.Memory, 0));

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image.Handle;
	viewInfo.viewType = cube ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : (layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D);
	viewInfo.format = format;
	viewInfo.subresourceRange = { (VkImageAspectFlags)(format == CaptureDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT), 0, 1, 0, layers };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &image.View));
	image.Width = width;
	image.Height = height;
}

void EnvMapVulkanProbes::DestroyImage(Image& image)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	if (image.View) vkDestroyImageView(device, image.View, nullptr);
	if (image.Handle) vkDestroyImage(device, image.Handle, nullptr);
	if (image.Memory) vkFreeMemory(device, image.Memory, nullptr);
	image = Image();
}

void EnvMapVulkanProbes::CreateAtlases(const glm::ivec3& counts)
{
	m_Counts = counts;
	const uint32_t tilesX = (uint32_t)(counts.x * counts.y), tilesY = (uint32_t)counts.z;
	// Written by compute passes, sampled by the mesh shaders
	const VkImageUsageFlags usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	CreateImage(m_Irradiance, IrradianceFormat, tilesX * (IrradianceTexels + 2), tilesY * (IrradianceTexels + 2), usage);
	CreateImage(m_Visibility, VisibilityFormat, tilesX * (VisibilityTexels + 2), tilesY * (VisibilityTexels + 2), usage);
	CreateImage(m_ProbeData, ProbeDataFormat, tilesX, tilesY, usage);

	// Into the layout they keep (see the class comment)
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	std::array<VkImageMemoryBarrier, 3> barriers = {};
	const VkImage images[3] = { m_Irradiance.Handle, m_Visibility.Handle, m_ProbeData.Handle };
	for (size_t i = 0; i < barriers.size(); i++)
	{
		barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].image = images[i];
		barriers[i].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	}
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), barriers.data());
	vulkanDevice->FlushCommandBuffer(commandBuffer);

	m_Filled = false;
	m_UpdateSetsWritten = false;
	m_Cursor = 0;
}

void EnvMapVulkanProbes::DestroyAtlases()
{
	DestroyImage(m_Irradiance);
	DestroyImage(m_Visibility);
	DestroyImage(m_ProbeData);
	m_Counts = glm::ivec3(0);
}

bool EnvMapVulkanProbes::Update(const EnvMapVulkanProbeVolumeSettings& settings, const VkDescriptorImageInfo& environmentIrradiance,
	const VkDescriptorImageInfo& environmentRadiance, float environmentRotation, uint32_t environmentVersion, uint64_t sceneVersion,
	uint64_t animationVersion)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	m_Settings = settings;
	m_EnvironmentRotation = environmentRotation;

	// Without a probe volume the (smallest) atlases stay as they are: nothing reads them
	bool replaced = false;
	const glm::ivec3 counts = settings.Exists ? settings.GetCounts() : m_Counts;
	if (counts != m_Counts)
	{
		vkDeviceWaitIdle(device); // the last frame may still sample the old atlases
		DestroyAtlases();
		CreateAtlases(counts);
		replaced = true;
	}

	if (m_ResetRequested)
	{
		m_ResetRequested = false;
		m_Filled = false;
		m_UpdatesLeft = 0;
		m_BakeLeft = 0;
	}

	// Probes that captured the scene aren't thrown back to the environment's light when the environment or the volume
	// changes: the next updates take them from what they hold to the new light
	const glm::vec3 spacing = settings.GetSpacing();
	const bool current = m_Filled && (m_Captured || (m_FilledRotation == environmentRotation && m_FilledEnvironmentVersion == environmentVersion &&
		(!settings.Exists || m_FilledSpacing == spacing)));
	if (!current)
	{
		vkDeviceWaitIdle(device); // the last frame may still sample the atlases this writes
		FillFromEnvironment(environmentIrradiance, environmentRotation);
		m_Filled = true;
		m_Captured = false;
		m_FilledSpacing = spacing;
		m_FilledRotation = environmentRotation;
		m_FilledEnvironmentVersion = environmentVersion;
	}

	if (m_CaptureSupported && (!m_UpdateSetsWritten || m_UpdateSetsEnvironmentVersion != environmentVersion))
	{
		vkDeviceWaitIdle(device); // the last frame may still use the sets
		WriteUpdateDescriptors(environmentRadiance);
		m_UpdateSetsWritten = true;
		m_UpdateSetsEnvironmentVersion = environmentVersion;
	}

	ScheduleUpdate(sceneVersion, animationVersion, !current);
	return replaced;
}

uint32_t EnvMapVulkanProbes::GetSettleRounds(float hysteresis)
{
	// After n updates hysteresis^n of the old value is left
	const float rounds = hysteresis > 0.0f ? std::ceil(std::log(0.02f) / std::log(hysteresis)) : 1.0f;
	return (uint32_t)glm::clamp(rounds, 4.0f, 64.0f);
}

void EnvMapVulkanProbes::ScheduleUpdate(uint64_t sceneVersion, uint64_t animationVersion, bool restarted)
{
	m_BatchCount = 0;
	const uint32_t total = GetProbeCount();
	if (!m_Settings.Exists || !m_CaptureSupported || total == 0)
	{
		m_UpdatesLeft = 0;
		m_BakeLeft = 0;
		m_BakeRequested = false;
		return;
	}

	// What the probes hold depends on the scene and on where the probes stand
	uint64_t version = HashValue(14695981039346656037ull, sceneVersion);
	version = HashValue(version, m_Settings.GetOrigin());
	version = HashValue(version, m_Settings.GetSpacing());
	version = HashValue(version, m_Counts);
	if (version != m_CapturedVersion || restarted)
	{
		m_CapturedVersion = version;
		m_Changed = true;
	}
	if (animationVersion != m_CapturedAnimationVersion)
	{
		m_CapturedAnimationVersion = animationVersion;
		m_Animated = true;
	}

	if (m_BakeRequested)
	{
		m_BakeRequested = false;
		m_BakeTotal = m_BakeLeft = BakeRounds * total;
		m_UpdatesLeft = 0;
		m_SinceChange = 0;
		m_Changed = m_Animated = false; // a change during the bake still counts
	}

	uint32_t count = 0;
	float hysteresis = 0.0f;
	if (m_BakeLeft > 0)
	{
		count = std::min({ CaptureSlots, total, m_BakeLeft });
		// Half of the rounds let the light travel, the other half are averaged
		hysteresis = GetRoundHysteresis(m_SinceChange / total, BakeRounds / 2, 0.98f);
		m_BakeLeft -= count;
	}
	else if (m_Settings.AutoUpdate)
	{
		const uint32_t settleUpdates = GetSettleRounds(m_Settings.GetHysteresis()) * total;
		if (m_Changed)
		{
			m_SinceChange = 0;
			m_UpdatesLeft = PropagationRounds * total + settleUpdates;
		}
		else if (m_Animated)
		{
			m_UpdatesLeft = std::max(m_UpdatesLeft, settleUpdates);
		}
		m_Changed = m_Animated = false;
		count = std::min({ (uint32_t)m_Settings.GetProbesPerFrame(), total, m_UpdatesLeft });
		hysteresis = GetRoundHysteresis(m_SinceChange / total, PropagationRounds, m_Settings.GetHysteresis());
		m_UpdatesLeft -= count;
	}
	else if (m_UpdatesLeft > 0)
	{
		// Switched off on the way: what was left stays to do
		m_UpdatesLeft = 0;
		m_Changed = true;
	}
	if (count == 0)
	{
		return;
	}

	// No probe twice in a batch (count <= total): two slots would write the same tile
	m_BatchFirst = m_Cursor % total;
	m_BatchCount = count;
	m_BatchHysteresis = hysteresis;
	m_Cursor = (m_BatchFirst + count) % total;
	m_SinceChange = std::min(m_SinceChange + count, 1000u * total); // stays far below the largest number
	m_Captured = true;

	// A new rotation of the ray directions, any rotation equally likely (a uniformly random unit quaternion)
	std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
	const float u1 = uniform(m_Random), u2 = uniform(m_Random) * glm::two_pi<float>(), u3 = uniform(m_Random) * glm::two_pi<float>();
	const glm::quat rotation(std::sqrt(1.0f - u1) * std::sin(u2), std::sqrt(1.0f - u1) * std::cos(u2), std::sqrt(u1) * std::sin(u3), std::sqrt(u1) * std::cos(u3));
	m_BatchRotation = glm::mat3_cast(rotation);
}

float EnvMapVulkanProbes::GetRoundHysteresis(uint32_t round, uint32_t propagationRounds, float settledHysteresis)
{
	// The light a probe holds is also what the surfaces around it are lit by in the next capture, so old light keeps
	// going around: each round only takes away the part the surfaces swallow, and the more of its old value a probe
	// keeps, the slower that goes. So after a change the first rounds keep nothing: a round is then one more bounce
	// of the new light, and one bounce less of the old.
	if (round < propagationRounds)
	{
		return 0.0f;
	}
	// Then the rounds are averaged (the n-th keeps (n - 1) / n of the value so far), which takes the noise of the
	// single rounds out, until the average is as slow as the settled hysteresis
	const float averaged = (float)(round - propagationRounds + 1);
	return std::min(1.0f - 1.0f / (averaged + 1.0f), settledHysteresis);
}

EnvMapVulkanProbes::UpdateStatus EnvMapVulkanProbes::GetStatus() const
{
	UpdateStatus status;
	status.Supported = m_CaptureSupported;
	status.Captured = m_Captured;
	status.Baking = m_BakeLeft > 0 || m_BakeRequested;
	status.BakeProgress = m_BakeTotal > 0 && !m_BakeRequested ? 1.0f - (float)m_BakeLeft / (float)m_BakeTotal : 0.0f;
	status.UpdatesLeft = m_UpdatesLeft;
	status.OutOfDate = (m_Changed || m_Animated) && !m_Settings.AutoUpdate && m_BakeLeft == 0;
	status.ProbesThisFrame = m_BatchCount;
	return status;
}

glm::vec3 EnvMapVulkanProbes::GetCaptureProbePosition(uint32_t slot) const
{
	// As BatchProbe in Include/ProbeUpdate.glslh
	const int index = (int)((m_BatchFirst + slot) % GetProbeCount());
	const glm::ivec3 probe(index % m_Counts.x, (index / m_Counts.x) % m_Counts.y, index / (m_Counts.x * m_Counts.y));
	return m_Settings.GetOrigin() + glm::vec3(probe) * m_Settings.GetSpacing();
}

void EnvMapVulkanProbes::CreateCaptureResources()
{
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkDevice device = vulkanDevice->GetVulkanDevice();
	m_CaptureSupported = vulkanDevice->GetEnabledFeatures().imageCubeArray;
	if (!m_CaptureSupported)
	{
		Log::GetLogger()->warn("Probe volume: the probes can't capture the scene, the GPU doesn't support cube map arrays (imageCubeArray)");
		return;
	}

	// The cubes (drawn into, then sampled by the resample pass), their depth, and the ray buffer
	const uint32_t layers = 6 * CaptureSlots;
	const VkImageUsageFlags cubeUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	CreateImage(m_CaptureRadiance, CaptureRadianceFormat, CaptureResolution, CaptureResolution, cubeUsage, layers, true);
	CreateImage(m_CaptureDistance, CaptureDistanceFormat, CaptureResolution, CaptureResolution, cubeUsage, layers, true);
	CreateImage(m_CaptureDepth, CaptureDepthFormat, CaptureResolution, CaptureResolution, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	CreateImage(m_RayBuffer, RayBufferFormat, EnvMapVulkanProbeVolumeSettings::MaxRaysPerProbe, CaptureSlots, VK_IMAGE_USAGE_STORAGE_BIT);

	// Into the layouts they have between the updates
	{
		VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
		std::array<VkImageMemoryBarrier, 3> barriers = {};
		const VkImage images[3] = { m_CaptureRadiance.Handle, m_CaptureDistance.Handle, m_RayBuffer.Handle };
		for (size_t i = 0; i < barriers.size(); i++)
		{
			const bool rayBuffer = i == 2;
			barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | (rayBuffer ? VK_ACCESS_SHADER_WRITE_BIT : 0);
			barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barriers[i].newLayout = rayBuffer ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barriers[i].image = images[i];
			barriers[i].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, rayBuffer ? 1 : layers };
		}
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
			(uint32_t)barriers.size(), barriers.data());
		vulkanDevice->FlushCommandBuffer(commandBuffer);
	}

	// The render pass of one cube face: radiance, distance, depth. All cleared; the two colors are kept and end up
	// readable for the resample pass.
	{
		std::array<VkAttachmentDescription, 3> attachments = {};
		const VkFormat formats[3] = { CaptureRadianceFormat, CaptureDistanceFormat, CaptureDepthFormat };
		for (size_t i = 0; i < attachments.size(); i++)
		{
			const bool depth = i == 2;
			attachments[i].format = formats[i];
			attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[i].storeOp = depth ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
			attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[i].finalLayout = depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		const VkAttachmentReference colorReferences[2] = { { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } };
		const VkAttachmentReference depthReference = { 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 2;
		subpass.pColorAttachments = colorReferences;
		subpass.pDepthStencilAttachment = &depthReference;

		// Before: the previous update's resample pass read the cubes, the previous face wrote the shared depth image.
		// After: this update's resample pass reads the cubes.
		std::array<VkSubpassDependency, 2> dependencies = {};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = (uint32_t)attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();
		VK_CHECK_RESULT_H2M(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_CaptureRenderPass));
	}

	// A framebuffer per cube face: that layer of the two cubes, and the depth image
	m_CaptureLayerViews.resize(2 * layers);
	m_CaptureFramebuffers.resize(layers);
	for (uint32_t layer = 0; layer < layers; layer++)
	{
		const Image* cubes[2] = { &m_CaptureRadiance, &m_CaptureDistance };
		const VkFormat formats[2] = { CaptureRadianceFormat, CaptureDistanceFormat };
		for (uint32_t i = 0; i < 2; i++)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = cubes[i]->Handle;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = formats[i];
			viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, layer, 1 };
			VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &m_CaptureLayerViews[i * layers + layer]));
		}
		const VkImageView views[3] = { m_CaptureLayerViews[layer], m_CaptureLayerViews[layers + layer], m_CaptureDepth.View };
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_CaptureRenderPass;
		framebufferInfo.attachmentCount = 3;
		framebufferInfo.pAttachments = views;
		framebufferInfo.width = CaptureResolution;
		framebufferInfo.height = CaptureResolution;
		framebufferInfo.layers = 1;
		VK_CHECK_RESULT_H2M(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_CaptureFramebuffers[layer]));
	}

	CreateCapturePipeline(false);
	CreateCapturePipeline(true);

	// Set 1 of the capture pipelines (the static shader's: the skinned one declares it identically). Its uniform buffer
	// here; its shadow map in SetCaptureSunShadowMap.
	H2M::RefH2M<H2M::VulkanShaderH2M> captureShader = H2M::RendererH2M::GetShaderLibrary()->Get("ProbeCapture_Static").As<H2M::VulkanShaderH2M>();
	m_CaptureSet = captureShader->CreateDescriptorSets(CaptureDescriptorSet);
	VkWriteDescriptorSet sunWrite = *captureShader->GetDescriptorSet("CaptureSun", CaptureDescriptorSet);
	sunWrite.dstSet = m_CaptureSet.DescriptorSets[0];
	sunWrite.descriptorCount = 1;
	sunWrite.pBufferInfo = &captureShader->GetUniformBuffer(0, CaptureDescriptorSet).Descriptor;
	vkUpdateDescriptorSets(device, 1, &sunWrite, 0, nullptr);
	SetCaptureSun(glm::mat4(1.0f), false, 0.0f, 1, 0.0f);

	H2M::RefH2M<H2M::ShaderLibraryH2M> shaders = H2M::RendererH2M::GetShaderLibrary();
	m_ResamplePipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(shaders->Get("ProbeResample"));
	m_BlendIrradiancePipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(shaders->Get("ProbeBlendIrradiance"));
	m_BlendVisibilityPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(shaders->Get("ProbeBlendVisibility"));
	m_BorderPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(shaders->Get("ProbeBorders"));
	m_ResampleSet = m_ResamplePipeline->GetShader()->CreateDescriptorSets();
	m_BlendIrradianceSet = m_BlendIrradiancePipeline->GetShader()->CreateDescriptorSets();
	m_BlendVisibilitySet = m_BlendVisibilityPipeline->GetShader()->CreateDescriptorSets();
	m_BorderSet = m_BorderPipeline->GetShader()->CreateDescriptorSets();
	m_UpdateSetsWritten = false;
}

void EnvMapVulkanProbes::DestroyCaptureResources()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	for (int i = 0; i < 2; i++)
	{
		if (m_CapturePipelines[i]) vkDestroyPipeline(device, m_CapturePipelines[i], nullptr);
		if (m_CaptureLayouts[i]) vkDestroyPipelineLayout(device, m_CaptureLayouts[i], nullptr);
		m_CapturePipelines[i] = VK_NULL_HANDLE;
		m_CaptureLayouts[i] = VK_NULL_HANDLE;
	}
	for (VkFramebuffer framebuffer : m_CaptureFramebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	for (VkImageView view : m_CaptureLayerViews)
	{
		vkDestroyImageView(device, view, nullptr);
	}
	m_CaptureFramebuffers.clear();
	m_CaptureLayerViews.clear();
	if (m_CaptureRenderPass) vkDestroyRenderPass(device, m_CaptureRenderPass, nullptr);
	m_CaptureRenderPass = VK_NULL_HANDLE;
	DestroyImage(m_CaptureRadiance);
	DestroyImage(m_CaptureDistance);
	DestroyImage(m_CaptureDepth);
	DestroyImage(m_RayBuffer);
	m_ResamplePipeline = m_BlendIrradiancePipeline = m_BlendVisibilityPipeline = m_BorderPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>();
	m_CaptureSet = m_ResampleSet = m_BlendIrradianceSet = m_BlendVisibilitySet = m_BorderSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
	m_CaptureSupported = false;
}

void EnvMapVulkanProbes::CreateCapturePipeline(bool skinned)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get(skinned ? "ProbeCapture_Anim" : "ProbeCapture_Static").As<H2M::VulkanShaderH2M>();
	const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();

	// The shader's sets (0 the per-frame set, 1 the capture's, 2 the bones of a skinned model) and one push constant
	// block for both stages
	std::vector<VkDescriptorSetLayout> setLayouts = shader->GetAllDescriptorSetLayouts();
	VkPushConstantRange pushRange = { CapturePushStages, 0, (uint32_t)sizeof(CapturePush) };
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = (uint32_t)setLayouts.size();
	layoutInfo.pSetLayouts = setLayouts.data();
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushRange;
	VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_CaptureLayouts[skinned ? 1 : 0]));

	// The models' vertices (ModelH2M's VertexH2M and AnimatedVertex), with only the attributes the capture reads
	VkVertexInputBindingDescription binding = { 0, (uint32_t)(skinned ? sizeof(H2M::AnimatedVertex) : sizeof(H2M::VertexH2M)), VK_VERTEX_INPUT_RATE_VERTEX };
	std::vector<VkVertexInputAttributeDescription> attributes;
	if (skinned)
	{
		attributes.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(H2M::AnimatedVertex, Position) });
		attributes.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(H2M::AnimatedVertex, Normal) });
		attributes.push_back({ 5, 0, VK_FORMAT_R32G32B32A32_SINT, (uint32_t)offsetof(H2M::AnimatedVertex, IDs) });
		attributes.push_back({ 6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, (uint32_t)offsetof(H2M::AnimatedVertex, Weights) });
	}
	else
	{
		attributes.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(H2M::VertexH2M, Position) });
		attributes.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(H2M::VertexH2M, Normal) });
	}
	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &binding;
	vertexInput.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
	vertexInput.pVertexAttributeDescriptions = attributes.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// No culling: a probe inside a mesh must see the backs of its faces (the fragment stage marks them)
	VkPipelineRasterizationStateCreateInfo rasterization = {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_NONE;
	rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	std::array<VkPipelineColorBlendAttachmentState, 2> blendAttachments = {};
	for (VkPipelineColorBlendAttachmentState& blendAttachment : blendAttachments)
	{
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}
	VkPipelineColorBlendStateCreateInfo colorBlend = {};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.attachmentCount = (uint32_t)blendAttachments.size();
	colorBlend.pAttachments = blendAttachments.data();

	std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = (uint32_t)stages.size();
	pipelineInfo.pStages = stages.data();
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterization;
	pipelineInfo.pMultisampleState = &multisample;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_CaptureLayouts[skinned ? 1 : 0];
	pipelineInfo.renderPass = m_CaptureRenderPass;
	pipelineInfo.subpass = 0;
	VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_CapturePipelines[skinned ? 1 : 0]));
}

void EnvMapVulkanProbes::WriteUpdateDescriptors(const VkDescriptorImageInfo& environmentRadiance)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	// Sampled: the capture cubes (nearest: a ray reads the one surface it hits) and the environment. Storage images: the
	// view and the layout (no sampler).
	const VkDescriptorImageInfo captureRadiance = { m_NearestSampler, m_CaptureRadiance.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	const VkDescriptorImageInfo captureDistance = { m_NearestSampler, m_CaptureDistance.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	const VkDescriptorImageInfo rayBuffer = { VK_NULL_HANDLE, m_RayBuffer.View, VK_IMAGE_LAYOUT_GENERAL };
	const VkDescriptorImageInfo irradiance = { VK_NULL_HANDLE, m_Irradiance.View, VK_IMAGE_LAYOUT_GENERAL };
	const VkDescriptorImageInfo visibility = { VK_NULL_HANDLE, m_Visibility.View, VK_IMAGE_LAYOUT_GENERAL };

	std::vector<VkWriteDescriptorSet> writes;
	auto add = [&writes](H2M::RefH2M<H2M::VulkanComputePipelineH2M> pipeline, const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set,
		const char* name, const VkDescriptorImageInfo* info)
	{
		VkWriteDescriptorSet write = *pipeline->GetShader()->GetDescriptorSet(name);
		write.dstSet = set.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = info;
		writes.push_back(write);
	};
	add(m_ResamplePipeline, m_ResampleSet, "u_CaptureRadiance", &captureRadiance);
	add(m_ResamplePipeline, m_ResampleSet, "u_CaptureDistance", &captureDistance);
	add(m_ResamplePipeline, m_ResampleSet, "u_EnvRadianceTex", &environmentRadiance);
	add(m_ResamplePipeline, m_ResampleSet, "o_RayBuffer", &rayBuffer);
	add(m_BlendIrradiancePipeline, m_BlendIrradianceSet, "u_RayBuffer", &rayBuffer);
	add(m_BlendIrradiancePipeline, m_BlendIrradianceSet, "u_Irradiance", &irradiance);
	add(m_BlendVisibilityPipeline, m_BlendVisibilitySet, "u_RayBuffer", &rayBuffer);
	add(m_BlendVisibilityPipeline, m_BlendVisibilitySet, "u_Visibility", &visibility);
	add(m_BorderPipeline, m_BorderSet, "u_Irradiance", &irradiance);
	add(m_BorderPipeline, m_BorderSet, "u_Visibility", &visibility);
	vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void EnvMapVulkanProbes::SetCaptureSunShadowMap(const VkDescriptorImageInfo& shadowMap)
{
	if (!m_CaptureSupported)
	{
		return;
	}
	H2M::RefH2M<H2M::VulkanShaderH2M> captureShader = H2M::RendererH2M::GetShaderLibrary()->Get("ProbeCapture_Static").As<H2M::VulkanShaderH2M>();
	VkWriteDescriptorSet write = *captureShader->GetDescriptorSet("u_CaptureSunShadowMap", CaptureDescriptorSet);
	write.dstSet = m_CaptureSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pImageInfo = &shadowMap;
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
}

void EnvMapVulkanProbes::SetCaptureSun(const glm::mat4& viewProjection, bool drawn, float texelWorldSize, uint32_t resolution, float normalBias)
{
	if (!m_CaptureSupported)
	{
		return;
	}
	CaptureSunUB ub;
	ub.ViewProjection = viewProjection;
	ub.Params = glm::vec4(drawn ? 1.0f : 0.0f, texelWorldSize, 1.0f / (float)std::max(resolution, 1u), normalBias);
	H2M::RefH2M<H2M::VulkanShaderH2M> captureShader = H2M::RendererH2M::GetShaderLibrary()->Get("ProbeCapture_Static").As<H2M::VulkanShaderH2M>();
	void* ubPtr = captureShader->MapUniformBuffer(0, CaptureDescriptorSet);
	memcpy(ubPtr, &ub, sizeof(ub));
	captureShader->UnmapUniformBuffer(0, CaptureDescriptorSet);
}

glm::mat4 EnvMapVulkanProbes::BeginCaptureFace(VkCommandBuffer commandBuffer, uint32_t slot, uint32_t face)
{
	// The near plane: close enough that a probe standing next to a wall doesn't look through it
	const glm::vec3 spacing = m_Settings.GetSpacing();
	const float nearPlane = glm::clamp(std::min(spacing.x, std::min(spacing.y, spacing.z)) * 0.01f, 0.005f, 0.05f);
	const glm::mat4 viewProjection = ComputeCubeFaceViewProjections(GetCaptureProbePosition(slot), nearPlane, CaptureFarPlane)[face];

	// Nothing seen: no light, the distance of a miss
	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].color = { { CaptureMissDistance, 0.0f, 0.0f, 0.0f } };
	clearValues[2].depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = m_CaptureRenderPass;
	beginInfo.framebuffer = m_CaptureFramebuffers[slot * 6 + face];
	beginInfo.renderArea.extent = { CaptureResolution, CaptureResolution };
	beginInfo.clearValueCount = (uint32_t)clearValues.size();
	beginInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	EnvMapVulkanProfiler::CountRenderPass(CaptureResolution, CaptureResolution);

	VkViewport viewport = { 0.0f, 0.0f, (float)CaptureResolution, (float)CaptureResolution, 0.0f, 1.0f };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	VkRect2D scissor = { { 0, 0 }, { CaptureResolution, CaptureResolution } };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	return viewProjection;
}

void EnvMapVulkanProbes::EndCaptureFace(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

void EnvMapVulkanProbes::RecordBlend(VkCommandBuffer commandBuffer)
{
	if (!HasCapture())
	{
		return;
	}
	const uint32_t rays = (uint32_t)m_Settings.GetRaysPerProbe();

	ProbeUpdatePush push;
	for (int i = 0; i < 3; i++)
	{
		push.RayRotation[i] = glm::vec4(m_BatchRotation[i], 0.0f);
	}
	push.Counts = glm::ivec4(m_Counts, (int)m_BatchFirst);
	push.Batch = glm::ivec4((int)m_BatchCount, (int)rays, 0, 0);
	// The visibility stores distances up to what the environment fill gives every probe (FillFromEnvironment)
	push.Params = glm::vec4(m_BatchHysteresis, IrradianceGamma, 1.5f * glm::length(m_Settings.GetSpacing()), m_EnvironmentRotation);

	// The images these passes write stay in one layout, so the steps are kept in order by memory barriers alone
	auto barrier = [commandBuffer](VkPipelineStageFlags sourceStages, VkAccessFlags sourceAccess, VkPipelineStageFlags destinationStages, VkAccessFlags destinationAccess)
	{
		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = sourceAccess;
		memoryBarrier.dstAccessMask = destinationAccess;
		vkCmdPipelineBarrier(commandBuffer, sourceStages, destinationStages, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	};
	auto dispatch = [commandBuffer, &push](H2M::RefH2M<H2M::VulkanComputePipelineH2M> pipeline, const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set,
		uint32_t groupsX, uint32_t groupsY)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetVulkanPipeline());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetVulkanPipelineLayout(), 0, 1, set.DescriptorSets.data(), 0, nullptr);
		vkCmdPushConstants(commandBuffer, pipeline->GetVulkanPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
		vkCmdDispatch(commandBuffer, groupsX, groupsY, 1);
	};
	const VkAccessFlags readWrite = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	const VkPipelineStageFlags compute = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	const VkPipelineStageFlags shading = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	// The cubes into the ray buffer (after whatever read it last): a thread per ray, 64 per group, a row of groups per slot
	barrier(compute, readWrite, compute, readWrite);
	dispatch(m_ResamplePipeline, m_ResampleSet, (rays + 63) / 64, m_BatchCount);

	// The rays into the tiles: after the ray buffer is written, and after the capture's fragment shaders read the atlases
	barrier(shading, readWrite, compute, readWrite);
	dispatch(m_BlendIrradiancePipeline, m_BlendIrradianceSet, m_BatchCount, 1);
	dispatch(m_BlendVisibilityPipeline, m_BlendVisibilitySet, m_BatchCount, 4); // a tile's four quarters

	// The tiles' borders, after their interiors are written
	barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, readWrite);
	dispatch(m_BorderPipeline, m_BorderSet, m_BatchCount, 1);

	// And the atlases are ready for the shaders that light the scene
	barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, shading, VK_ACCESS_SHADER_READ_BIT);
}

void EnvMapVulkanProbes::FillFromEnvironment(const VkDescriptorImageInfo& environmentIrradiance, float environmentRotation)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_FillPipeline->GetShader();

	// Storage image descriptors: the view and the layout (no sampler)
	const VkDescriptorImageInfo storageInfos[3] = {
		{ VK_NULL_HANDLE, m_Irradiance.View, VK_IMAGE_LAYOUT_GENERAL },
		{ VK_NULL_HANDLE, m_Visibility.View, VK_IMAGE_LAYOUT_GENERAL },
		{ VK_NULL_HANDLE, m_ProbeData.View, VK_IMAGE_LAYOUT_GENERAL } };
	const char* storageNames[3] = { "o_Irradiance", "o_Visibility", "o_ProbeData" };
	std::array<VkWriteDescriptorSet, 4> writes;
	writes[0] = *shader->GetDescriptorSet("u_EnvIrradianceTex");
	writes[0].dstSet = m_FillDescriptorSet.DescriptorSets[0];
	writes[0].descriptorCount = 1;
	writes[0].pImageInfo = &environmentIrradiance;
	for (uint32_t i = 0; i < 3; i++)
	{
		writes[1 + i] = *shader->GetDescriptorSet(storageNames[i]);
		writes[1 + i].dstSet = m_FillDescriptorSet.DescriptorSets[0];
		writes[1 + i].descriptorCount = 1;
		writes[1 + i].pImageInfo = &storageInfos[i];
	}
	vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);

	// Every probe sees farther than any point its cell's corners can ask about (a cell's diagonal), so no probe is
	// counted as hidden
	const float seenDistance = 1.5f * glm::length(m_Settings.GetSpacing());
	const glm::vec4 params(environmentRotation, seenDistance, IrradianceGamma, 0.0f);

	// One thread per texel of the largest atlas, in groups of 8 x 8 (see ProbeEnvFill.glsl)
	m_FillPipeline->Begin();
	m_FillPipeline->SetPushConstants(&params, sizeof(params));
	m_FillPipeline->Dispatch(m_FillDescriptorSet.DescriptorSets[0], (m_Visibility.Width + 7) / 8, (m_Visibility.Height + 7) / 8, 1);
	m_FillPipeline->End();
	vkQueueWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetComputeQueue());
}

EnvMapVulkanProbes::ProbeVolumeUB EnvMapVulkanProbes::GetUniforms() const
{
	ProbeVolumeUB ub;
	ub.Origin = glm::vec4(m_Settings.GetOrigin(), m_Settings.LightsScene() ? 1.0f : 0.0f);
	ub.Spacing = glm::vec4(m_Settings.GetSpacing(), IrradianceGamma);
	ub.Counts = glm::ivec4(m_Counts, 0);
	ub.Biases = glm::vec4(m_Settings.NormalBias, m_Settings.ViewBias, 0.0f, 0.0f);
	ub.AtlasSizes = glm::vec4((float)m_Irradiance.Width, (float)m_Irradiance.Height, (float)m_Visibility.Width, (float)m_Visibility.Height);
	return ub;
}

void EnvMapVulkanProbes::CreateSpherePipeline(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get("ProbeSpheres").As<H2M::VulkanShaderH2M>();
	const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();

	// Set 0 only (the shader declares it as the PBR shaders do, so the per-frame set fits), and the vertex stage's push constants
	VkDescriptorSetLayout setLayout = shader->GetDescriptorSetLayout(H2M::VulkanShaderH2M::FrameDescriptorSet);
	VkPushConstantRange pushRange = { VK_SHADER_STAGE_VERTEX_BIT, 0, (uint32_t)sizeof(ProbeSpheresPush) };
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &setLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushRange;
	VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_SpherePipelineLayout));

	// No vertex buffers: the vertex stage makes the squares from the vertex and instance index
	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterization = {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_NONE;
	rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Solid balls: tested against the scene's depth and written into it (the fragment stage gives the ball's depth)
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo colorBlend = {};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.attachmentCount = 1;
	colorBlend.pAttachments = &blendAttachment;

	std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = (uint32_t)stages.size();
	pipelineInfo.pStages = stages.data();
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterization;
	pipelineInfo.pMultisampleState = &multisample;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_SpherePipelineLayout;
	// The scene framebuffer's render pass (its continue render pass, used after the water's scene copy, is compatible)
	pipelineInfo.renderPass = sceneFramebuffer.As<H2M::VulkanFramebufferH2M>()->GetRenderPass();
	pipelineInfo.subpass = 0;
	VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_SpherePipeline));
}

void EnvMapVulkanProbes::RecordSpheres(VkCommandBuffer commandBuffer, VkDescriptorSet frameDescriptorSet, const glm::mat4& cameraView)
{
	if (!ShowsSpheres())
	{
		return;
	}
	// The camera's right and up directions in the world: the first two rows of the view matrix's rotation
	ProbeSpheresPush push;
	push.OriginRadius = glm::vec4(m_Settings.GetOrigin(), m_Settings.ProbeRadius);
	push.Spacing = glm::vec4(m_Settings.GetSpacing(), (float)m_Settings.Show);
	push.Counts = glm::ivec4(m_Counts, 0);
	push.CameraRight = glm::vec4(cameraView[0][0], cameraView[1][0], cameraView[2][0], 0.0f);
	push.CameraUp = glm::vec4(cameraView[0][1], cameraView[1][1], cameraView[2][1], 0.0f);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SpherePipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SpherePipelineLayout, H2M::VulkanShaderH2M::FrameDescriptorSet, 1,
		&frameDescriptorSet, 0, nullptr);
	vkCmdPushConstants(commandBuffer, m_SpherePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
	vkCmdDraw(commandBuffer, 6, GetProbeCount(), 0, 0);
	EnvMapVulkanProfiler::CountDraw(2 * GetProbeCount());
}
