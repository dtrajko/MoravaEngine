#include "EnvMapVulkanGlass.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h"
#include "H2M/Renderer/RenderPassH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/Log.h"

#include <algorithm>
#include <array>
#include <vector>


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
	VkAccessFlags srcAccess, VkAccessFlags dstAccess, uint32_t baseLevel = 0, uint32_t levelCount = 1)
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
	barrier.subresourceRange = { aspects, baseLevel, levelCount, 0, 1 };
	return barrier;
}

void EnvMapVulkanGlass::Create(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get("Glass_Static").As<H2M::VulkanShaderH2M>();

	// The static mesh vertex layout (ModelH2M's Vertex), into the scene framebuffer (its continue render pass is compatible)
	H2M::PipelineSpecificationH2M pipelineSpecification;
	pipelineSpecification.Layout = {
		{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
		{ H2M::ShaderDataTypeH2M::Float3, "a_Normal" },
		{ H2M::ShaderDataTypeH2M::Float3, "a_Tangent" },
		{ H2M::ShaderDataTypeH2M::Float3, "a_Binormal" },
		{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
	};
	pipelineSpecification.Shader = shader;
	H2M::RenderPassSpecificationH2M renderPassSpec;
	renderPassSpec.TargetFramebuffer = sceneFramebuffer;
	pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
	pipelineSpecification.DebugName = "Glass";
	m_Pipeline = H2M::PipelineH2M::Create(pipelineSpecification);

	// The copies: the scene framebuffer's formats (vkCmdCopyImage copies between equal formats)
	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = sceneFramebuffer.As<H2M::VulkanFramebufferH2M>();
	H2M_CORE_ASSERT(framebuffer->GetSpecification().CopySource, "Glass needs a scene framebuffer created with CopySource");
	m_ColorFormat = framebuffer->GetColorVulkanFormat();
	m_DepthFormat = framebuffer->GetDepthVulkanFormat();

	VkFormatProperties colorProperties;
	vkGetPhysicalDeviceFormatProperties(H2M::VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetVulkanPhysicalDevice(), m_ColorFormat, &colorProperties);
	const VkFormatFeatureFlags blitFeatures = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
	if ((colorProperties.optimalTilingFeatures & blitFeatures) != blitFeatures)
	{
		Log::GetLogger()->error("Glass: the scene color format {0} can't be blitted with linear filtering; rough glass won't be blurred", (int)m_ColorFormat);
	}

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxLod = (float)MaxCopyLevels;
	samplerInfo.maxAnisotropy = 1.0f;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_ColorSampler));
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.maxLod = 0.0f;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_DepthSampler));

	m_DescriptorSet = shader->CreateDescriptorSets(SceneCopySet);
	Resize(framebuffer->GetWidth(), framebuffer->GetHeight());
}

void EnvMapVulkanGlass::Destroy()
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
	m_DescriptorSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
}

void EnvMapVulkanGlass::CreateCopyImage(CopyImage& copy, VkFormat format, VkImageAspectFlags aspect, uint32_t levels)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = { m_Width, m_Height, 1 };
	imageInfo.mipLevels = levels;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	// The color's levels are filled by blits from the level above (a source and a destination)
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | (levels > 1 ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK_RESULT_H2M(vkCreateImage(device, &imageInfo, nullptr, &copy.Image));

	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(device, copy.Image, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("GlassSceneCopy"));
	allocator.Allocate(requirements, &copy.Memory);
	VK_CHECK_RESULT_H2M(vkBindImageMemory(device, copy.Image, copy.Memory, 0));

	// A depth / stencil image is sampled through a depth-only view
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = copy.Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = { aspect, 0, levels, 0, 1 };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &copy.View));
}

void EnvMapVulkanGlass::DestroyCopyImages()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	for (CopyImage* copy : { &m_Color, &m_Depth })
	{
		if (copy->View) vkDestroyImageView(device, copy->View, nullptr);
		if (copy->Image) vkDestroyImage(device, copy->Image, nullptr);
		if (copy->Memory) vkFreeMemory(device, copy->Memory, nullptr);
		*copy = CopyImage();
	}
}

void EnvMapVulkanGlass::Resize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0 || (width == m_Width && height == m_Height && m_Color.Image))
	{
		return;
	}
	// The old copies may still be read by a frame in flight, and the descriptor set is rewritten below
	vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
	DestroyCopyImages();
	m_Width = width;
	m_Height = height;
	uint32_t fullChain = 1;
	for (uint32_t size = std::max(width, height); size > 1; size /= 2)
	{
		fullChain++;
	}
	m_ColorLevels = std::min(fullChain, MaxCopyLevels);
	CreateCopyImage(m_Color, m_ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_ColorLevels);
	CreateCopyImage(m_Depth, m_DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	// Into the layout the glass samples them in, so the descriptors are valid before the first copy
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	std::array<VkImageMemoryBarrier, 2> barriers = {
		ImageBarrier(m_Color.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
			VK_ACCESS_SHADER_READ_BIT, 0, m_ColorLevels),
		ImageBarrier(m_Depth.Image, TransitionAspects(m_DepthFormat, true), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
			VK_ACCESS_SHADER_READ_BIT) };
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
		(uint32_t)barriers.size(), barriers.data());
	vulkanDevice->FlushCommandBuffer(commandBuffer);

	WriteDescriptors();
}

void EnvMapVulkanGlass::WriteDescriptors()
{
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_Pipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	VkDescriptorImageInfo colorInfo = { m_ColorSampler, m_Color.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	VkDescriptorImageInfo depthInfo = { m_DepthSampler, m_Depth.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	std::array<VkWriteDescriptorSet, 2> writes;
	writes[0] = *shader->GetDescriptorSet("u_SceneColor", SceneCopySet);
	writes[0].dstSet = GetSceneCopySet();
	writes[0].descriptorCount = 1;
	writes[0].pImageInfo = &colorInfo;
	writes[1] = *shader->GetDescriptorSet("u_SceneDepth", SceneCopySet);
	writes[1].dstSet = GetSceneCopySet();
	writes[1].descriptorCount = 1;
	writes[1].pImageInfo = &depthInfo;
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void EnvMapVulkanGlass::CopyScene(VkCommandBuffer commandBuffer, H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer)
{
	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = sceneFramebuffer.As<H2M::VulkanFramebufferH2M>();
	VkImage sceneColor = framebuffer->GetColorVulkanImage();
	VkImage sceneDepth = framebuffer->GetDepthVulkanImage();
	const VkImageAspectFlags depthAspects = TransitionAspects(m_DepthFormat, true);

	// The scene pass (or its continue pass) left color in SHADER_READ_ONLY and depth in DEPTH_STENCIL_ATTACHMENT (see
	// VulkanFramebufferH2M). The copies' previous contents aren't needed.
	{
		std::array<VkImageMemoryBarrier, 4> barriers = {
			ImageBarrier(sceneColor, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),
			ImageBarrier(sceneDepth, depthAspects, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),
			ImageBarrier(m_Color.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
				VK_ACCESS_TRANSFER_WRITE_BIT, 0, m_ColorLevels),
			ImageBarrier(m_Depth.Image, depthAspects, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT) };
		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), barriers.data());
	}

	const uint32_t width = std::min(m_Width, framebuffer->GetWidth());
	const uint32_t height = std::min(m_Height, framebuffer->GetHeight());
	VkImageCopy region = {};
	region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	region.extent = { width, height, 1 };
	vkCmdCopyImage(commandBuffer, sceneColor, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_Color.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // the depth only (a copy of a depth / stencil image takes one aspect)
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	vkCmdCopyImage(commandBuffer, sceneDepth, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_Depth.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// The scene's images back to the layouts the continue render pass expects
	{
		std::array<VkImageMemoryBarrier, 2> barriers = {
			ImageBarrier(sceneColor, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
			ImageBarrier(sceneDepth, depthAspects, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT) };
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			0, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), barriers.data());
	}

	// The color's mip chain: each level a half-size linear blit of the one above it
	int32_t levelWidth = (int32_t)width, levelHeight = (int32_t)height;
	for (uint32_t level = 1; level < m_ColorLevels; level++)
	{
		VkImageMemoryBarrier toSource = ImageBarrier(m_Color.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, level - 1, 1);
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toSource);

		const int32_t nextWidth = std::max(levelWidth / 2, 1), nextHeight = std::max(levelHeight / 2, 1);
		VkImageBlit blit = {};
		blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1 };
		blit.srcOffsets[1] = { levelWidth, levelHeight, 1 };
		blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1 };
		blit.dstOffsets[1] = { nextWidth, nextHeight, 1 };
		vkCmdBlitImage(commandBuffer, m_Color.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_Color.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit, VK_FILTER_LINEAR);
		levelWidth = nextWidth;
		levelHeight = nextHeight;
	}

	// Everything to the layout the glass samples it in: the levels read by a blit are in TRANSFER_SRC, the last in TRANSFER_DST
	{
		std::vector<VkImageMemoryBarrier> barriers;
		if (m_ColorLevels > 1)
		{
			barriers.push_back(ImageBarrier(m_Color.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, 0, m_ColorLevels - 1));
		}
		barriers.push_back(ImageBarrier(m_Color.Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, m_ColorLevels - 1, 1));
		barriers.push_back(ImageBarrier(m_Depth.Image, depthAspects, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT));
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
			(uint32_t)barriers.size(), barriers.data());
	}
}
