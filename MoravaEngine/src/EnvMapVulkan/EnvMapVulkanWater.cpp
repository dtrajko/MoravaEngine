#include "EnvMapVulkanWater.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanIndexBufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Platform/Vulkan/VulkanVertexBufferH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"
#include "H2M/Renderer/PipelineH2M.h"
#include "H2M/Renderer/RenderPassH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/Log.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cmath>
#include <cstring>


// std140 layout of the WaterSettings uniform block in Water.glsl (set 1, binding 1)
struct WaterSettingsUB
{
	glm::vec4 WaveOffsets;
	glm::vec3 ScatterColor;
	float Roughness;
	float WaveScale1;
	float WaveScale2;
	float WaveStrength;
	float ReflectionStrength;
	glm::vec4 DepthParams;     // depth -> view distance: distance = -y / (depth * z - x); w = the water height
	glm::vec3 Absorption;      // per meter, per color (Beer-Lambert)
	float RefractionStrength;
	float EdgeSoftness;
	float FoamAmount;
	float FoamWidth;
	float Padding;
};
static_assert(sizeof(WaterSettingsUB) == 96, "WaterSettingsUB must match the std140 layout of WaterSettings in Water.glsl");

static constexpr uint32_t WaterSet = 1; // the water's own descriptor set (set 0 is the per-frame set)

static bool HasStencil(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT;
}

// The aspects a layout transition of the image covers (both of a depth / stencil image)
static VkImageAspectFlags TransitionAspects(VkFormat format, bool depth)
{
	if (!depth)
	{
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
	return HasStencil(format) ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_DEPTH_BIT;
}

static VkImageMemoryBarrier ImageBarrier(VkImage image, VkImageAspectFlags aspects, VkImageLayout oldLayout, VkImageLayout newLayout,
	VkAccessFlags srcAccess, VkAccessFlags dstAccess)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = { aspects, 0, 1, 0, 1 };
	return barrier;
}

glm::mat4 EnvMapVulkanWaterSettings::GetTransform() const
{
	return glm::translate(glm::mat4(1.0f), glm::vec3(Center.x, Height, Center.y)) * glm::scale(glm::mat4(1.0f), glm::vec3(Size.x, 1.0f, Size.y));
}

void EnvMapVulkanWater::Create(H2M::RefH2M<H2M::FramebufferH2M> targetFramebuffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get("Water").As<H2M::VulkanShaderH2M>();

	H2M::PipelineSpecificationH2M pipelineSpecification;
	pipelineSpecification.Layout = {
		{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
	};
	pipelineSpecification.Shader = shader;
	pipelineSpecification.BackfaceCulling = false; // seen from above and from below
	H2M::RenderPassSpecificationH2M renderPassSpec;
	renderPassSpec.TargetFramebuffer = targetFramebuffer;
	pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
	pipelineSpecification.DebugName = "Water";
	m_Pipeline = H2M::PipelineH2M::Create(pipelineSpecification);

	// A unit square in XZ, facing up (counter-clockwise seen from above)
	glm::vec3 vertices[4] = { { -0.5f, 0.0f, -0.5f }, { -0.5f, 0.0f, 0.5f }, { 0.5f, 0.0f, 0.5f }, { 0.5f, 0.0f, -0.5f } };
	uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
	m_VertexBuffer = H2M::VertexBufferH2M::Create(vertices, sizeof(vertices));
	m_IndexBuffer = H2M::IndexBufferH2M::Create(indices, sizeof(indices));

	// Linear data (the slopes of the waves), not sRGB color
	m_NormalMap = H2M::Texture2D_H2M::Create("Textures/water/waterNormal.png", false);

	m_DescriptorSet = shader->CreateDescriptorSets(WaterSet);
	std::array<VkWriteDescriptorSet, 2> writes;
	writes[0] = *shader->GetDescriptorSet("u_WaterNormalMap", WaterSet);
	writes[0].dstSet = m_DescriptorSet.DescriptorSets[0];
	writes[0].descriptorCount = 1;
	writes[0].pImageInfo = &m_NormalMap.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
	writes[1] = *shader->GetDescriptorSet("WaterSettings", WaterSet);
	writes[1].dstSet = m_DescriptorSet.DescriptorSets[0];
	writes[1].descriptorCount = 1;
	writes[1].pBufferInfo = &shader->GetUniformBuffer(1, WaterSet).Descriptor;
	vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);

	// The scene copies: the same formats as the scene framebuffer (vkCmdCopyImage copies between equal formats)
	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = targetFramebuffer.As<H2M::VulkanFramebufferH2M>();
	H2M_CORE_ASSERT(framebuffer->GetSpecification().CopySource, "The water needs a scene framebuffer created with CopySource");
	m_ColorFormat = framebuffer->GetColorVulkanFormat();
	m_DepthFormat = framebuffer->GetDepthVulkanFormat();

	VkFormatProperties depthProperties;
	vkGetPhysicalDeviceFormatProperties(H2M::VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetVulkanPhysicalDevice(), m_DepthFormat, &depthProperties);
	if (!(depthProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
	{
		Log::GetLogger()->error("Water: the depth format {0} can't be sampled; refraction depth will be wrong", (int)m_DepthFormat);
	}

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxLod = 0.0f;
	samplerInfo.maxAnisotropy = 1.0f;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_ColorSampler));
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_DepthSampler));

	Resize(targetFramebuffer->GetWidth(), targetFramebuffer->GetHeight());
}

void EnvMapVulkanWater::Destroy()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	DestroyCopyImages();
	if (m_ColorSampler)
	{
		vkDestroySampler(device, m_ColorSampler, nullptr);
		m_ColorSampler = VK_NULL_HANDLE;
	}
	if (m_DepthSampler)
	{
		vkDestroySampler(device, m_DepthSampler, nullptr);
		m_DepthSampler = VK_NULL_HANDLE;
	}
	m_Pipeline = H2M::RefH2M<H2M::PipelineH2M>();
	m_VertexBuffer = H2M::RefH2M<H2M::VertexBufferH2M>();
	m_IndexBuffer = H2M::RefH2M<H2M::IndexBufferH2M>();
	m_NormalMap = H2M::RefH2M<H2M::Texture2D_H2M>();
	m_DescriptorSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
}

void EnvMapVulkanWater::CreateCopyImage(CopyImage& copy, VkFormat format, VkImageAspectFlags aspect)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = { m_CopyWidth, m_CopyHeight, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK_RESULT_H2M(vkCreateImage(device, &imageInfo, nullptr, &copy.Image));

	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(device, copy.Image, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("WaterSceneCopy"));
	allocator.Allocate(requirements, &copy.Memory);
	VK_CHECK_RESULT_H2M(vkBindImageMemory(device, copy.Image, copy.Memory, 0));

	// A depth / stencil image is sampled through a depth-only view
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = copy.Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = { aspect, 0, 1, 0, 1 };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &copy.View));
	copy.Format = format;
}

void EnvMapVulkanWater::DestroyCopyImages()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	for (CopyImage* copy : { &m_SceneColor, &m_SceneDepth })
	{
		if (copy->View) vkDestroyImageView(device, copy->View, nullptr);
		if (copy->Image) vkDestroyImage(device, copy->Image, nullptr);
		if (copy->Memory) vkFreeMemory(device, copy->Memory, nullptr);
		*copy = CopyImage();
	}
}

void EnvMapVulkanWater::Resize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0 || (width == m_CopyWidth && height == m_CopyHeight && m_SceneColor.Image))
	{
		return;
	}
	// The old copies may still be read by a frame in flight, and the descriptor set is rewritten below
	vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
	DestroyCopyImages();
	m_CopyWidth = width;
	m_CopyHeight = height;
	CreateCopyImage(m_SceneColor, m_ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	CreateCopyImage(m_SceneDepth, m_DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	// Into the layout the water samples them in, so the descriptors are valid before the first copy
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	std::array<VkImageMemoryBarrier, 2> barriers = {
		ImageBarrier(m_SceneColor.Image, TransitionAspects(m_ColorFormat, false), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT),
		ImageBarrier(m_SceneDepth.Image, TransitionAspects(m_DepthFormat, true), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT) };
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
		(uint32_t)barriers.size(), barriers.data());
	vulkanDevice->FlushCommandBuffer(commandBuffer);

	WriteSceneCopyDescriptors();
}

void EnvMapVulkanWater::WriteSceneCopyDescriptors()
{
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_Pipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	VkDescriptorImageInfo colorInfo = { m_ColorSampler, m_SceneColor.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	VkDescriptorImageInfo depthInfo = { m_DepthSampler, m_SceneDepth.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	std::array<VkWriteDescriptorSet, 2> writes;
	writes[0] = *shader->GetDescriptorSet("u_SceneColor", WaterSet);
	writes[0].dstSet = m_DescriptorSet.DescriptorSets[0];
	writes[0].descriptorCount = 1;
	writes[0].pImageInfo = &colorInfo;
	writes[1] = *shader->GetDescriptorSet("u_SceneDepth", WaterSet);
	writes[1].dstSet = m_DescriptorSet.DescriptorSets[0];
	writes[1].descriptorCount = 1;
	writes[1].pImageInfo = &depthInfo;
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void EnvMapVulkanWater::Update(const EnvMapVulkanWaterSettings& settings, float deltaTime, const glm::mat4& projection)
{
	// Each layer scrolls at WaveSpeed world units per second (the ripples a bit slower), in its own direction; the
	// offsets are in normal map tiles, so they wrap at 1 (no precision loss over time)
	float angle1 = glm::radians(settings.WaveDirection);
	float angle2 = glm::radians(settings.WaveDirection + 70.0f);
	glm::vec2 velocity1 = glm::vec2(std::cos(angle1), std::sin(angle1)) * settings.WaveSpeed / std::max(settings.WaveScale1, 0.01f);
	glm::vec2 velocity2 = glm::vec2(std::cos(angle2), std::sin(angle2)) * settings.WaveSpeed * 0.7f / std::max(settings.WaveScale2, 0.01f);
	m_WaveOffsets += glm::vec4(velocity1, velocity2) * deltaTime;
	m_WaveOffsets = m_WaveOffsets - glm::floor(m_WaveOffsets);

	WaterSettingsUB ub;
	ub.WaveOffsets = m_WaveOffsets;
	ub.ScatterColor = settings.ScatterColor;
	ub.Roughness = settings.Roughness;
	ub.WaveScale1 = std::max(settings.WaveScale1, 0.01f);
	ub.WaveScale2 = std::max(settings.WaveScale2, 0.01f);
	ub.WaveStrength = settings.WaveStrength;
	ub.ReflectionStrength = settings.ReflectionStrength;
	// A perspective projection: depth = (A z + B) / (C z), z the view space depth (negative in front of the camera), so
	// z = B / (depth C - A). The depth buffer holds the normalized device depth as it is (Vulkan doesn't remap it).
	ub.DepthParams = glm::vec4(projection[2][2], projection[3][2], projection[2][3], settings.Height);
	// Beer-Lambert: Transmittance is left after Clarity meters, so the absorption per meter is -ln(Transmittance) / Clarity
	ub.Absorption = -glm::log(glm::clamp(settings.Transmittance, glm::vec3(0.001f), glm::vec3(1.0f))) / std::max(settings.Clarity, 0.01f);
	ub.RefractionStrength = settings.RefractionStrength;
	ub.EdgeSoftness = std::max(settings.EdgeSoftness, 0.001f);
	ub.FoamAmount = settings.FoamAmount;
	ub.FoamWidth = std::max(settings.FoamWidth, 0.001f);
	ub.Padding = 0.0f;

	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_Pipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	void* data = shader->MapUniformBuffer(1, WaterSet);
	memcpy(data, &ub, sizeof(ub));
	shader->UnmapUniformBuffer(1, WaterSet);
}

void EnvMapVulkanWater::CopyScene(VkCommandBuffer commandBuffer, H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer)
{
	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = sceneFramebuffer.As<H2M::VulkanFramebufferH2M>();
	VkImage sceneColor = framebuffer->GetColorVulkanImage();
	VkImage sceneDepth = framebuffer->GetDepthVulkanImage();
	VkImageAspectFlags colorAspects = TransitionAspects(m_ColorFormat, false);
	VkImageAspectFlags depthAspects = TransitionAspects(m_DepthFormat, true);

	// The render pass left color in SHADER_READ_ONLY and depth in DEPTH_STENCIL_ATTACHMENT (see VulkanFramebufferH2M)
	{
		std::array<VkImageMemoryBarrier, 4> barriers = {
			ImageBarrier(sceneColor, colorAspects, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),
			ImageBarrier(sceneDepth, depthAspects, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),
			// The previous contents are not needed (only the previous frame's water read them)
			ImageBarrier(m_SceneColor.Image, colorAspects, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT),
			ImageBarrier(m_SceneDepth.Image, depthAspects, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT) };
		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), barriers.data());
	}

	VkImageCopy region = {};
	region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.extent = { std::min(m_CopyWidth, framebuffer->GetWidth()), std::min(m_CopyHeight, framebuffer->GetHeight()), 1 };
	vkCmdCopyImage(commandBuffer, sceneColor, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_SceneColor.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // the depth only (a copy of a depth / stencil image takes one aspect)
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	vkCmdCopyImage(commandBuffer, sceneDepth, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_SceneDepth.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// Back to the layouts the continue render pass expects, and the copies to the layout the water samples them in
	{
		std::array<VkImageMemoryBarrier, 4> barriers = {
			ImageBarrier(sceneColor, colorAspects, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
			ImageBarrier(sceneDepth, depthAspects, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
			ImageBarrier(m_SceneColor.Image, colorAspects, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT),
			ImageBarrier(m_SceneDepth.Image, depthAspects, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT) };
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), barriers.data());
	}
}

void EnvMapVulkanWater::Record(VkCommandBuffer commandBuffer, VkDescriptorSet frameDescriptorSet, const EnvMapVulkanWaterSettings& settings)
{
	H2M::RefH2M<H2M::VulkanPipelineH2M> pipeline = m_Pipeline.As<H2M::VulkanPipelineH2M>();
	VkPipelineLayout layout = pipeline->GetVulkanPipelineLayout();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetVulkanPipeline());
	// Set 0 is declared identically in Water.glsl and the PBR shaders, so the per-frame set is valid here too
	VkDescriptorSet sets[2] = { frameDescriptorSet, m_DescriptorSet.DescriptorSets[0] };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 2, sets, 0, nullptr);

	glm::mat4 transform = settings.GetTransform();
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);

	VkBuffer vertexBuffer = m_VertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.As<H2M::VulkanIndexBufferH2M>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
}

bool RaycastWater(const EnvMapVulkanWaterSettings& settings, const glm::vec3& origin, const glm::vec3& direction, float& t)
{
	if (!settings.Enabled || std::abs(direction.y) < 1e-6f)
	{
		return false;
	}
	t = (settings.Height - origin.y) / direction.y;
	if (t <= 0.0f)
	{
		return false;
	}
	glm::vec3 hit = origin + direction * t;
	return std::abs(hit.x - settings.Center.x) <= settings.Size.x * 0.5f && std::abs(hit.z - settings.Center.y) <= settings.Size.y * 0.5f;
}
