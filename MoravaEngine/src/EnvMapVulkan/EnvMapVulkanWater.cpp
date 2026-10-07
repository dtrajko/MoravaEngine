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
#include "EnvMapVulkanProfiler.h"

#include <glm/gtc/constants.hpp>
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
	float PlanarReflection;    // 1: the planar reflection image of this frame is valid
	float ReflectionDistortion;
	float TransparencyFromBelow;
	float WaterRotation;       // radians around Y
	float Padding;
	glm::mat4 InverseViewProjection; // the camera's clip space -> world (the volume pass rebuilds positions from depth)
	glm::vec4 WaterBounds;           // the rectangle: center x, center z, half size x, half size z
	glm::vec4 GerstnerParams;        // x = the number of Gerstner waves
	glm::vec4 GerstnerWaves[8];      // per wave: (direction x, direction z, wave number, amplitude), (phase, steepness, 0, 0)
};
static_assert(sizeof(WaterSettingsUB) == 336, "WaterSettingsUB must match the std140 layout of WaterSettings in Include/WaterSettings.glslh");

// The swell (Gerstner waves): the longest wave goes along WaveDirection; the others are shorter and turned off it, so the
// crests cross and the pattern doesn't repeat visibly. Their heights are in proportion to their lengths (the same
// steepness), together SwellHeight.
static constexpr int SwellWaveCount = 4;
static constexpr float SwellLengths[SwellWaveCount] = { 1.0f, 0.61f, 0.37f, 0.23f }; // of SwellLength
static constexpr float SwellAngles[SwellWaveCount] = { 0.0f, 32.0f, -41.0f, 67.0f }; // degrees off WaveDirection

// The surface's grid: cells per side over the water rectangle (the waves move its vertices)
static constexpr uint32_t WaterGridCells = 256;

static constexpr uint32_t WaterSet = 1; // the water's own descriptor set (set 0 is the per-frame set)
static constexpr uint32_t FrameSet = 0; // the per-frame set (the reflection's own, with the mirrored camera)

// A host visible uniform buffer of the given size (like VulkanShaderH2M's own)
static void CreateUniformBuffer(H2M::VulkanShaderH2M::UniformBufferH2M& buffer, uint32_t size)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	VK_CHECK_RESULT_H2M(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer.Buffer));
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(device, buffer.Buffer, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("WaterReflection"));
	allocator.Allocate(requirements, &buffer.Memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK_RESULT_H2M(vkBindBufferMemory(device, buffer.Buffer, buffer.Memory, 0));
	buffer.Size = size;
	buffer.Descriptor = { buffer.Buffer, 0, size };
}

static void WriteUniformBuffer(const H2M::VulkanShaderH2M::UniformBufferH2M& buffer, const void* data, uint32_t size)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	void* mapped;
	VK_CHECK_RESULT_H2M(vkMapMemory(device, buffer.Memory, 0, size, 0, &mapped));
	memcpy(mapped, data, size);
	vkUnmapMemory(device, buffer.Memory);
}

static void DestroyUniformBuffer(H2M::VulkanShaderH2M::UniformBufferH2M& buffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	if (buffer.Buffer) vkDestroyBuffer(device, buffer.Buffer, nullptr);
	if (buffer.Memory) vkFreeMemory(device, buffer.Memory, nullptr);
	buffer.Buffer = VK_NULL_HANDLE;
	buffer.Memory = VK_NULL_HANDLE;
}

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

// Beer-Lambert: Transmittance is left after Clarity meters, so the absorption per meter is -ln(Transmittance) / Clarity
static glm::vec3 GetAbsorption(const EnvMapVulkanWaterSettings& settings)
{
	return -glm::log(glm::clamp(settings.Transmittance, glm::vec3(0.001f), glm::vec3(1.0f))) / std::max(settings.Clarity, 0.01f);
}

static constexpr uint32_t CausticsResolution = 1024; // texels per side of the caustics map
// Grid cells per side over the map (a margin is added around it): a cell is a few texels, so a triangle stays wider than a
// texel until the waves squeeze it a lot (a smaller one lands on a single texel, or on none: bright dots and gaps)
static constexpr uint32_t CausticsGridCells = 384;

static bool IsCausticsOn(const EnvMapVulkanWaterSettings& settings, const glm::vec3& sunDirection)
{
	return settings.Caustics && settings.CausticsStrength > 0.0f && sunDirection.y > 0.01f;
}

// The caustics map's square: centered on the camera and kept over the water (on the water's center along a side shorter
// than the square), its corner snapped to the grid's cell size, so the grid samples the waves at the same places while the
// camera moves (no shimmering). Returns the corner (x, z), the size and the cell size.
static glm::vec4 GetCausticsRegion(const EnvMapVulkanWaterSettings& settings, const glm::vec3& cameraPosition)
{
	glm::vec2 halfExtents = settings.GetWorldHalfExtents(); // the square is aligned with the world: kept inside the water's bounding box
	float size = std::max(std::min(settings.CausticsArea, 2.0f * std::max(halfExtents.x, halfExtents.y)), 0.5f);
	float cellSize = size / (float)CausticsGridCells;
	glm::vec2 camera = glm::vec2(cameraPosition.x, cameraPosition.z);
	glm::vec2 center;
	for (int axis = 0; axis < 2; axis++)
	{
		float freedom = halfExtents[axis] - size * 0.5f; // how far the square's center can be from the water's
		center[axis] = freedom > 0.0f ? glm::clamp(camera[axis], settings.Center[axis] - freedom, settings.Center[axis] + freedom) : settings.Center[axis];
	}
	glm::vec2 corner = glm::floor((center - size * 0.5f) / cellSize) * cellSize;
	return glm::vec4(corner, size, cellSize);
}

void FillWaterSceneData(const EnvMapVulkanWaterSettings& settings, const glm::vec3& cameraPosition, const glm::vec3& sunDirection,
	EnvMapVulkanSceneDataGPU& data)
{
	if (!settings.Enabled)
	{
		return;
	}
	data.WaterVolumeBounds = glm::vec4(settings.Center.x, settings.Center.y, settings.Size.x * 0.5f, settings.Size.y * 0.5f);
	data.WaterVolumeParams = glm::vec4(settings.Height, 0.0f, 1.0f, glm::radians(settings.Rotation));
	data.WaterVolumeAbsorption = glm::vec4(GetAbsorption(settings), 0.0f);
	data.WaterVolumeScatter = glm::vec4(settings.ScatterColor, 0.0f);
	if (IsCausticsOn(settings, sunDirection))
	{
		glm::vec4 region = GetCausticsRegion(settings, cameraPosition);
		data.CausticsRegion = glm::vec4(region.x, region.y, 1.0f / region.z, 10.0f); // fades out over the outer tenth
		data.CausticsParams = glm::vec4(settings.CausticsStrength, std::max(settings.CausticsFocus, 0.05f), 0.0f, 0.0f);
	}
}

glm::mat4 EnvMapVulkanWaterSettings::GetTransform() const
{
	return glm::translate(glm::mat4(1.0f), glm::vec3(Center.x, Height, Center.y)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(Rotation), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(Size.x, 1.0f, Size.y));
}

glm::vec2 EnvMapVulkanWaterSettings::GetWorldHalfExtents() const
{
	float c = std::abs(std::cos(glm::radians(Rotation)));
	float s = std::abs(std::sin(glm::radians(Rotation)));
	return glm::vec2(c * Size.x + s * Size.y, s * Size.x + c * Size.y) * 0.5f;
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

	// The volume pass: a full-screen triangle over the whole image (no depth test: it rewrites every pixel from the copy)
	pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("WaterFog");
	pipelineSpecification.DepthTest = false;
	pipelineSpecification.DepthWrite = false;
	pipelineSpecification.DebugName = "WaterVolume";
	m_VolumePipeline = H2M::PipelineH2M::Create(pipelineSpecification);
	glm::vec3 triangle[3] = { { -1.0f, -1.0f, 0.0f }, { 3.0f, -1.0f, 0.0f }, { -1.0f, 3.0f, 0.0f } };
	m_FullscreenTriangle = H2M::VertexBufferH2M::Create(triangle, sizeof(triangle));

	// The wireframe: the surface's triangles as lines, over the surface (pulled toward the camera by a depth bias, so the
	// lines win against the surface they lie on), not hiding anything
	pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("WaterWireframe");
	pipelineSpecification.DepthTest = true;
	pipelineSpecification.DepthWrite = false;
	pipelineSpecification.Wireframe = true;
	pipelineSpecification.DepthBiasConstant = -4.0f;
	pipelineSpecification.DepthBiasSlope = -2.0f;
	pipelineSpecification.DebugName = "WaterWireframe";
	m_WireframePipeline = H2M::PipelineH2M::Create(pipelineSpecification);

	// A grid over the unit square in XZ, facing up (counter-clockwise seen from above): the waves move its vertices
	const uint32_t verticesPerSide = WaterGridCells + 1;
	std::vector<glm::vec3> vertices;
	vertices.reserve(verticesPerSide * verticesPerSide);
	for (uint32_t z = 0; z < verticesPerSide; z++)
	{
		for (uint32_t x = 0; x < verticesPerSide; x++)
		{
			vertices.push_back({ (float)x / WaterGridCells - 0.5f, 0.0f, (float)z / WaterGridCells - 0.5f });
		}
	}
	std::vector<uint32_t> indices;
	indices.reserve(WaterGridCells * WaterGridCells * 6);
	for (uint32_t z = 0; z < WaterGridCells; z++)
	{
		for (uint32_t x = 0; x < WaterGridCells; x++)
		{
			uint32_t corner = z * verticesPerSide + x; // the cell's corner at the smallest x and z
			uint32_t cell[6] = { corner, corner + verticesPerSide, corner + verticesPerSide + 1, corner + verticesPerSide + 1, corner + 1, corner };
			indices.insert(indices.end(), cell, cell + 6);
		}
	}
	m_VertexBuffer = H2M::VertexBufferH2M::Create(vertices.data(), (uint32_t)(vertices.size() * sizeof(glm::vec3)));
	m_IndexBuffer = H2M::IndexBufferH2M::Create(indices.data(), (uint32_t)(indices.size() * sizeof(uint32_t)));
	m_IndexCount = (uint32_t)indices.size();

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

	CreateReflection();
	CreateCaustics();
	Resize(targetFramebuffer->GetWidth(), targetFramebuffer->GetHeight());
}

void EnvMapVulkanWater::CreateCaustics()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	const VkFormat format = VK_FORMAT_R16_SFLOAT;

	// The map: one channel, the light relative to a flat surface (drawn into, then sampled by the PBR shaders)
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = { CausticsResolution, CausticsResolution, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK_RESULT_H2M(vkCreateImage(device, &imageInfo, nullptr, &m_CausticsImage));
	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(device, m_CausticsImage, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("WaterCaustics"));
	allocator.Allocate(requirements, &m_CausticsMemory);
	VK_CHECK_RESULT_H2M(vkBindImageMemory(device, m_CausticsImage, m_CausticsMemory, 0));

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_CausticsImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &m_CausticsView));

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxLod = 0.0f;
	samplerInfo.maxAnisotropy = 1.0f;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_CausticsSampler));
	m_CausticsDescriptor = { m_CausticsSampler, m_CausticsView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	// Cleared, drawn with additive blending, then left for the PBR shaders to sample
	VkAttachmentDescription attachment = {};
	attachment.format = format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // the previous frame's map is never needed
	attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	std::array<VkSubpassDependency, 2> dependencies = {};
	// The previous frame's shaders must be done reading the map before it is cleared and drawn again
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// The map must be written before the meshes' fragment shaders sample it
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();
	VK_CHECK_RESULT_H2M(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_CausticsRenderPass));

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = m_CausticsRenderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &m_CausticsView;
	framebufferInfo.width = CausticsResolution;
	framebufferInfo.height = CausticsResolution;
	framebufferInfo.layers = 1;
	VK_CHECK_RESULT_H2M(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_CausticsFramebuffer));

	// The pipeline: no vertex buffer (the grid comes from the vertex index), no culling (the waves can fold a triangle
	// over), additive blending (the light of all the triangles that land on a texel adds up)
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get("WaterCaustics").As<H2M::VulkanShaderH2M>();
	std::vector<VkDescriptorSetLayout> setLayouts = shader->GetAllDescriptorSetLayouts();
	std::vector<VkPushConstantRange> pushConstantRanges;
	for (const auto& range : shader->GetPushConstantRanges())
	{
		pushConstantRanges.push_back({ (VkShaderStageFlags)range.ShaderStage, range.Offset, range.Size });
	}
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = (uint32_t)setLayouts.size();
	layoutInfo.pSetLayouts = setLayouts.data();
	layoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
	layoutInfo.pPushConstantRanges = pushConstantRanges.data();
	VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_CausticsLayout));

	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkViewport viewport = { 0.0f, 0.0f, (float)CausticsResolution, (float)CausticsResolution, 0.0f, 1.0f };
	VkRect2D scissor = { { 0, 0 }, { CausticsResolution, CausticsResolution } };
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
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
	VkPipelineColorBlendAttachmentState blend = {};
	blend.blendEnable = VK_TRUE;
	blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend.colorBlendOp = VK_BLEND_OP_ADD;
	blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend.alphaBlendOp = VK_BLEND_OP_ADD;
	blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
	VkPipelineColorBlendStateCreateInfo colorBlend = {};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.attachmentCount = 1;
	colorBlend.pAttachments = &blend;
	const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();
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
	pipelineInfo.layout = m_CausticsLayout;
	pipelineInfo.renderPass = m_CausticsRenderPass;
	pipelineInfo.subpass = 0;
	VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_CausticsPipeline));

	// The normal map and the settings (for the swell): the water's own
	m_CausticsSet = shader->CreateDescriptorSets(0);
	H2M::RefH2M<H2M::VulkanShaderH2M> waterShader = m_Pipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	std::array<VkWriteDescriptorSet, 2> writes = { *shader->GetDescriptorSet("u_WaterNormalMap", 0), *shader->GetDescriptorSet("WaterSettings", 0) };
	writes[0].dstSet = writes[1].dstSet = m_CausticsSet.DescriptorSets[0];
	writes[0].descriptorCount = writes[1].descriptorCount = 1;
	writes[0].pImageInfo = &m_NormalMap.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
	writes[1].pBufferInfo = &waterShader->GetUniformBuffer(1, WaterSet).Descriptor;
	vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);

	// Valid to sample before the first caustics pass: cleared to 1 (the light of a flat surface)
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	VkImageMemoryBarrier barrier = ImageBarrier(m_CausticsImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	VkClearColorValue one = { { 1.0f, 1.0f, 1.0f, 1.0f } };
	vkCmdClearColorImage(commandBuffer, m_CausticsImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &one, 1, &viewInfo.subresourceRange);
	barrier = ImageBarrier(m_CausticsImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	vulkanDevice->FlushCommandBuffer(commandBuffer);
}

void EnvMapVulkanWater::DestroyCaustics()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	if (m_CausticsPipeline) vkDestroyPipeline(device, m_CausticsPipeline, nullptr);
	if (m_CausticsLayout) vkDestroyPipelineLayout(device, m_CausticsLayout, nullptr);
	if (m_CausticsFramebuffer) vkDestroyFramebuffer(device, m_CausticsFramebuffer, nullptr);
	if (m_CausticsRenderPass) vkDestroyRenderPass(device, m_CausticsRenderPass, nullptr);
	if (m_CausticsSampler) vkDestroySampler(device, m_CausticsSampler, nullptr);
	if (m_CausticsView) vkDestroyImageView(device, m_CausticsView, nullptr);
	if (m_CausticsImage) vkDestroyImage(device, m_CausticsImage, nullptr);
	if (m_CausticsMemory) vkFreeMemory(device, m_CausticsMemory, nullptr);
	m_CausticsPipeline = VK_NULL_HANDLE;
	m_CausticsLayout = VK_NULL_HANDLE;
	m_CausticsFramebuffer = VK_NULL_HANDLE;
	m_CausticsRenderPass = VK_NULL_HANDLE;
	m_CausticsSampler = VK_NULL_HANDLE;
	m_CausticsView = VK_NULL_HANDLE;
	m_CausticsImage = VK_NULL_HANDLE;
	m_CausticsMemory = VK_NULL_HANDLE;
	m_CausticsDescriptor = {};
	m_CausticsSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
	m_CausticsActive = false;
}

void EnvMapVulkanWater::CreateReflection()
{
	H2M::FramebufferSpecificationH2M spec;
	spec.Attachments = { H2M::ImageFormatH2M::RGBA16F, H2M::ImageFormatH2M::Depth }; // as the scene framebuffer (compatible render pass)
	spec.Width = 64; // sized in ResizeReflection
	spec.Height = 64;
	spec.DebugName = "WaterReflection";
	m_ReflectionFramebuffer = H2M::FramebufferH2M::Create(spec);

	// Set 0 of the reflection: the PBR shader's per-frame layout. Camera and SceneData are its own (the mirrored camera);
	// the other bindings are copied from the main per-frame set every frame (see Update).
	H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = H2M::RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static").As<H2M::VulkanShaderH2M>();
	m_ReflectionFrameSet = pbrShader->CreateDescriptorSets(FrameSet);
	CreateUniformBuffer(m_ReflectionCamera, sizeof(glm::mat4));
	CreateUniformBuffer(m_ReflectionSceneData, sizeof(EnvMapVulkanSceneDataGPU));
	std::array<VkWriteDescriptorSet, 2> writes;
	writes[0] = *pbrShader->GetDescriptorSet("Camera", FrameSet);
	writes[0].dstSet = m_ReflectionFrameSet.DescriptorSets[0];
	writes[0].descriptorCount = 1;
	writes[0].pBufferInfo = &m_ReflectionCamera.Descriptor;
	writes[1] = *pbrShader->GetDescriptorSet("SceneData", FrameSet);
	writes[1].dstSet = m_ReflectionFrameSet.DescriptorSets[0];
	writes[1].descriptorCount = 1;
	writes[1].pBufferInfo = &m_ReflectionSceneData.Descriptor;
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void EnvMapVulkanWater::ResizeReflection()
{
	if (!m_ReflectionFramebuffer || m_CopyWidth == 0 || m_CopyHeight == 0)
	{
		return;
	}
	uint32_t divisor = std::max(m_ReflectionDivisor, 1u);
	m_ReflectionFramebuffer->Resize(std::max(m_CopyWidth / divisor, 1u), std::max(m_CopyHeight / divisor, 1u)); // waits for the device
	WriteReflectionDescriptor();
}

void EnvMapVulkanWater::WriteReflectionDescriptor()
{
	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = m_ReflectionFramebuffer.As<H2M::VulkanFramebufferH2M>();

	// A new image is in an undefined layout until its first render pass: make it valid to sample (the water reads it even
	// in frames that don't draw the reflection; their settings tell the shader to ignore it)
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	VkImageMemoryBarrier barrier = ImageBarrier(framebuffer->GetColorVulkanImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT);
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	vulkanDevice->FlushCommandBuffer(commandBuffer);

	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_Pipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	VkWriteDescriptorSet write = *shader->GetDescriptorSet("u_ReflectionTexture", WaterSet);
	write.dstSet = m_DescriptorSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pImageInfo = &framebuffer->GetVulkanDescriptorInfo();
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
}

void EnvMapVulkanWater::Destroy()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	DestroyCopyImages();
	DestroyCaustics();
	DestroyUniformBuffer(m_ReflectionCamera);
	DestroyUniformBuffer(m_ReflectionSceneData);
	m_ReflectionFrameSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
	m_ReflectionFramebuffer = H2M::RefH2M<H2M::FramebufferH2M>();
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
	m_VolumePipeline = H2M::RefH2M<H2M::PipelineH2M>();
	m_WireframePipeline = H2M::RefH2M<H2M::PipelineH2M>();
	m_FullscreenTriangle = H2M::RefH2M<H2M::VertexBufferH2M>();
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
	ResizeReflection();
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

void EnvMapVulkanWater::Update(const EnvMapVulkanWaterSettings& settings, float deltaTime, const glm::mat4& view, const glm::mat4& projection,
	const glm::vec3& cameraPosition, float envMapRotation, const glm::vec3& sunDirection, VkDescriptorSet frameDescriptorSet)
{
	// Planar reflection: from above the water what is above the surface; from under the water what is under it (the
	// surface is a mirror there outside Snell's window)
	const bool cameraBelow = cameraPosition.y < settings.Height;
	m_ReflectionActive = settings.PlanarReflection;
	if (m_ReflectionActive)
	{
		uint32_t divisor = settings.ReflectionDivisor == 1 || settings.ReflectionDivisor == 4 ? settings.ReflectionDivisor : 2;
		if (divisor != m_ReflectionDivisor)
		{
			m_ReflectionDivisor = divisor;
			ResizeReflection();
		}

		// The camera mirrored in the water plane: the view of the world mirrored (y -> 2 h - y)
		const float h = settings.Height;
		glm::mat4 mirror = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, h, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f)) *
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -h, 0.0f));
		glm::mat4 reflectedView = view * mirror;

		// Oblique near plane (Lengyel, "Oblique View Frustum Depth Projection and Clipping"): the near plane becomes the
		// water plane, so nothing on the camera's side of the water gets into the reflection. Vulkan clips at depth 0, so
		// the plane goes to clip z = 0; the far plane is kept through the far corner of the frustum on the side the plane
		// faces. The plane is a few centimeters past the surface, so things standing in the water reflect to the waterline.
		glm::vec4 plane = cameraBelow ? glm::vec4(0.0f, -1.0f, 0.0f, h + 0.03f)  // keeps y <= h + 0.03
		                              : glm::vec4(0.0f, 1.0f, 0.0f, -(h - 0.03f)); // keeps y >= h - 0.03
		glm::vec4 C = glm::transpose(glm::inverse(reflectedView)) * plane; // in the mirrored camera's view space
		glm::mat4 obliqueProjection = projection;
		auto signOf = [](float v) { return v >= 0.0f ? 1.0f : -1.0f; };
		glm::vec4 q = glm::inverse(projection) * glm::vec4(signOf(C.x * projection[0][0]), signOf(C.y * projection[1][1]), 1.0f, 1.0f);
		float cq = glm::dot(C, q);
		if (cq > 1e-6f)
		{
			glm::vec4 row3 = glm::vec4(projection[0][3], projection[1][3], projection[2][3], projection[3][3]);
			glm::vec4 row2 = C * (glm::dot(row3, q) / cq);
			obliqueProjection[0][2] = row2.x;
			obliqueProjection[1][2] = row2.y;
			obliqueProjection[2][2] = row2.z;
			obliqueProjection[3][2] = row2.w;
		}
		// Flipped vertically: the mirror reversed the triangles' winding, the flip turns it back (and the water samples
		// the image flipped)
		glm::mat4 viewProjection = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f)) * obliqueProjection * reflectedView;
		WriteUniformBuffer(m_ReflectionCamera, &viewProjection, sizeof(glm::mat4));
		// The mirrored camera and the water (the light under it as in the scene pass, the caustics around the real camera);
		// under the water also the water volume, so the PBR shaders dim each mirrored mesh by the water between it and the
		// surface (the rays of the mirrored camera, above the surface, cross exactly that water)
		EnvMapVulkanSceneDataGPU sceneData;
		sceneData.CameraPosition = glm::vec3(cameraPosition.x, 2.0f * h - cameraPosition.y, cameraPosition.z);
		sceneData.EnvMapRotation = envMapRotation;
		FillWaterSceneData(settings, cameraPosition, sunDirection, sceneData);
		if (cameraBelow)
		{
			sceneData.WaterVolumeParams.y = 1.0f;
		}
		WriteUniformBuffer(m_ReflectionSceneData, &sceneData, sizeof(sceneData));

		// The rest of set 0 (environment maps, BRDF LUT, lights, shadows, caustics) as in the main per-frame set
		std::array<VkCopyDescriptorSet, 9> copies;
		for (uint32_t i = 0; i < (uint32_t)copies.size(); i++)
		{
			VkCopyDescriptorSet& copy = copies[i];
			copy = {};
			copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
			copy.srcSet = frameDescriptorSet;
			copy.srcBinding = 2 + i;
			copy.dstSet = m_ReflectionFrameSet.DescriptorSets[0];
			copy.dstBinding = 2 + i;
			copy.descriptorCount = 1;
		}
		vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 0, nullptr, (uint32_t)copies.size(), copies.data());
	}

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
	ub.Absorption = GetAbsorption(settings);
	ub.RefractionStrength = settings.RefractionStrength;
	ub.EdgeSoftness = std::max(settings.EdgeSoftness, 0.001f);
	ub.FoamAmount = settings.FoamAmount;
	ub.FoamWidth = std::max(settings.FoamWidth, 0.001f);
	ub.PlanarReflection = m_ReflectionActive ? 1.0f : 0.0f;
	ub.ReflectionDistortion = settings.ReflectionDistortion;
	ub.TransparencyFromBelow = glm::clamp(settings.TransparencyFromBelow, 0.0f, 1.0f);
	ub.WaterRotation = glm::radians(settings.Rotation);
	ub.Padding = 0.0f;
	ub.InverseViewProjection = glm::inverse(projection * view);
	ub.WaterBounds = glm::vec4(settings.Center.x, settings.Center.y, settings.Size.x * 0.5f, settings.Size.y * 0.5f);

	// The swell. Each wave travels at the speed of a deep water wave of its length (angular frequency sqrt(g k): a wave
	// twice as long is 1.4x faster); its phase is kept in 0..2 pi, so it doesn't lose precision over time. The steepness
	// is shared: Q k A of all the waves adds up to SwellSteepness, so at 1 the sharpest crests just close (no loops).
	std::memset(ub.GerstnerWaves, 0, sizeof(ub.GerstnerWaves));
	ub.GerstnerParams = glm::vec4(0.0f);
	if (settings.SwellHeight > 0.0f && settings.SwellLength > 0.0f)
	{
		float lengthSum = 0.0f;
		for (float ratio : SwellLengths)
		{
			lengthSum += ratio;
		}
		const float gravity = 9.81f;
		const float steepness = glm::clamp(settings.SwellSteepness, 0.0f, 1.0f);
		for (int i = 0; i < SwellWaveCount; i++)
		{
			float wavelength = std::max(settings.SwellLength * SwellLengths[i], 0.1f);
			float k = glm::two_pi<float>() / wavelength;
			float amplitude = settings.SwellHeight * SwellLengths[i] / lengthSum;
			float angle = glm::radians(settings.WaveDirection + SwellAngles[i]);
			m_SwellPhases[i] = std::fmod(m_SwellPhases[i] - std::sqrt(gravity * k) * deltaTime, glm::two_pi<float>()); // moves along the direction
			ub.GerstnerWaves[2 * i] = glm::vec4(std::cos(angle), std::sin(angle), k, amplitude);
			ub.GerstnerWaves[2 * i + 1] = glm::vec4(m_SwellPhases[i], steepness / (k * amplitude * SwellWaveCount), 0.0f, 0.0f);
		}
		ub.GerstnerParams.x = (float)SwellWaveCount;
	}

	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_Pipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	void* data = shader->MapUniformBuffer(1, WaterSet);
	memcpy(data, &ub, sizeof(ub));
	shader->UnmapUniformBuffer(1, WaterSet);

	// Caustics: the grid over the map's square (the same square FillWaterSceneData gives the PBR shaders), with a margin
	// around it as wide as the waves can move the light at the focus depth (light from there lands inside)
	m_CausticsActive = IsCausticsOn(settings, sunDirection);
	if (m_CausticsActive)
	{
		glm::vec4 region = GetCausticsRegion(settings, cameraPosition);
		float cellSize = region.w;
		float focus = std::max(settings.CausticsFocus, 0.05f);
		uint32_t marginCells = (uint32_t)glm::clamp(std::ceil(focus * 0.3f / cellSize), 4.0f, 192.0f);
		uint32_t cellsPerSide = CausticsGridCells + 2 * marginCells;
		m_CausticsGridVertices = cellsPerSide * cellsPerSide * 6;

		m_CausticsConstants.Region = glm::vec4(region.x, region.y, region.z, (float)CausticsResolution);
		m_CausticsConstants.Grid = glm::vec4(region.x - marginCells * cellSize, region.y - marginCells * cellSize, cellSize, (float)cellsPerSide);
		m_CausticsConstants.WaveOffsets = m_WaveOffsets;
		m_CausticsConstants.Waves = glm::vec4(ub.WaveScale1, ub.WaveScale2, settings.WaveStrength, 0.0f);
		m_CausticsConstants.Sun = glm::vec4(glm::normalize(sunDirection), focus);
		// The normal map's mip level whose texels are about a grid cell: smaller waves would only add noise between the samples
		float normalMapSize = (float)std::max(m_NormalMap->GetWidth(), 1u);
		m_CausticsConstants.Lod = glm::vec4(std::max(std::log2(cellSize * normalMapSize / ub.WaveScale1), 0.0f),
			std::max(std::log2(cellSize * normalMapSize / ub.WaveScale2), 0.0f), 0.0f, 0.0f);
	}
}

void EnvMapVulkanWater::RecordCaustics(VkCommandBuffer commandBuffer)
{
	if (!m_CausticsActive || !m_CausticsPipeline)
	{
		return;
	}
	VkClearValue clearValue = {};
	clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // no light until the triangles bring it
	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = m_CausticsRenderPass;
	beginInfo.framebuffer = m_CausticsFramebuffer;
	beginInfo.renderArea.extent = { CausticsResolution, CausticsResolution };
	beginInfo.clearValueCount = 1;
	beginInfo.pClearValues = &clearValue;
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	EnvMapVulkanProfiler::CountRenderPass(beginInfo.renderArea.extent.width, beginInfo.renderArea.extent.height);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CausticsPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CausticsLayout, 0, 1, &m_CausticsSet.DescriptorSets[0], 0, nullptr);
	vkCmdPushConstants(commandBuffer, m_CausticsLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(CausticsPushConstants), &m_CausticsConstants);
	vkCmdDraw(commandBuffer, m_CausticsGridVertices, 1, 0, 0);
	EnvMapVulkanProfiler::CountDraw(m_CausticsGridVertices / 3);
	vkCmdEndRenderPass(commandBuffer);
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

VkDescriptorSet EnvMapVulkanWater::BeginReflectionPass(VkCommandBuffer commandBuffer)
{
	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = m_ReflectionFramebuffer.As<H2M::VulkanFramebufferH2M>();
	uint32_t width = framebuffer->GetWidth(), height = framebuffer->GetHeight();

	// Cleared to transparent: where nothing is drawn (the sky), the water uses the environment map
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = framebuffer->GetRenderPass();
	beginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();
	beginInfo.renderArea.extent = { width, height };
	beginInfo.clearValueCount = 2;
	beginInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	EnvMapVulkanProfiler::CountRenderPass(beginInfo.renderArea.extent.width, beginInfo.renderArea.extent.height);

	VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	VkRect2D scissor = { { 0, 0 }, { width, height } };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	return m_ReflectionFrameSet.DescriptorSets[0];
}

void EnvMapVulkanWater::EndReflectionPass(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer); // leaves the image in SHADER_READ_ONLY, for the water
}

void EnvMapVulkanWater::RecordVolume(VkCommandBuffer commandBuffer, VkDescriptorSet frameDescriptorSet)
{
	H2M::RefH2M<H2M::VulkanPipelineH2M> pipeline = m_VolumePipeline.As<H2M::VulkanPipelineH2M>();
	VkPipelineLayout layout = pipeline->GetVulkanPipelineLayout();
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetVulkanPipeline());
	// Both sets are declared in WaterFog.glsl exactly as in Water.glsl, so the water's sets are valid here
	VkDescriptorSet sets[2] = { frameDescriptorSet, m_DescriptorSet.DescriptorSets[0] };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 2, sets, 0, nullptr);
	VkBuffer vertexBuffer = m_FullscreenTriangle.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	EnvMapVulkanProfiler::CountDraw(1);
}

void EnvMapVulkanWater::Record(VkCommandBuffer commandBuffer, VkDescriptorSet frameDescriptorSet, const EnvMapVulkanWaterSettings& settings,
	float wireframeLineWidth)
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
	vkCmdDrawIndexed(commandBuffer, m_IndexCount, 1, 0, 0, 0);
	EnvMapVulkanProfiler::CountDraw(m_IndexCount / 3);

	// The wireframe over it: the same grid and sets, the same transform (the vertex stage is shared)
	if (settings.Wireframe && m_WireframePipeline)
	{
		H2M::RefH2M<H2M::VulkanPipelineH2M> wireframe = m_WireframePipeline.As<H2M::VulkanPipelineH2M>();
		VkPipelineLayout wireframeLayout = wireframe->GetVulkanPipelineLayout();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe->GetVulkanPipeline());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframeLayout, 0, 2, sets, 0, nullptr);
		vkCmdPushConstants(commandBuffer, wireframeLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
		vkCmdSetLineWidth(commandBuffer, wireframeLineWidth); // a dynamic state of line-mode pipelines
		vkCmdDrawIndexed(commandBuffer, m_IndexCount, 1, 0, 0, 0);
		EnvMapVulkanProfiler::CountDraw(m_IndexCount / 3);
	}
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
	// In the rectangle's own frame (turned back by its rotation, as WaterLocalXZ in Include/WaterVolume.glslh)
	glm::vec3 hit = origin + direction * t;
	glm::vec2 offset = glm::vec2(hit.x, hit.z) - settings.Center;
	float angle = glm::radians(settings.Rotation);
	glm::vec2 local = glm::vec2(std::cos(angle) * offset.x - std::sin(angle) * offset.y, std::sin(angle) * offset.x + std::cos(angle) * offset.y);
	return std::abs(local.x) <= settings.Size.x * 0.5f && std::abs(local.y) <= settings.Size.y * 0.5f;
}
