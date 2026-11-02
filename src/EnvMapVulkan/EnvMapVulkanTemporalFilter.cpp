#include "EnvMapVulkanTemporalFilter.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <algorithm>
#include <array>


static constexpr VkFormat HistoryInfoFormat = VK_FORMAT_R16G16_SFLOAT;

void EnvMapVulkanComputeImage::Create(VkFormat format, uint32_t width, uint32_t height, uint32_t levels, float clearValue)
{
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkDevice device = vulkanDevice->GetVulkanDevice();
	Width = width;
	Height = height;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = { width, height, 1 };
	imageInfo.mipLevels = levels;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK_RESULT_H2M(vkCreateImage(device, &imageInfo, nullptr, &Handle));

	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(device, Handle, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("ComputeImage"));
	allocator.Allocate(requirements, &Memory);
	VK_CHECK_RESULT_H2M(vkBindImageMemory(device, Handle, Memory, 0));

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = Handle;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, levels, 0, 1 };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &View));
	LevelViews.resize(levels);
	for (uint32_t level = 0; level < levels; level++)
	{
		viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, level, 1, 0, 1 };
		VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &LevelViews[level]));
	}

	// Into the layout it keeps, cleared
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = Handle;
	barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, levels, 0, 1 };
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	const VkClearColorValue clearColor = { { clearValue, clearValue, clearValue, clearValue } };
	vkCmdClearColorImage(commandBuffer, Handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &barrier.subresourceRange);
	vulkanDevice->FlushCommandBuffer(commandBuffer);
}

void EnvMapVulkanComputeImage::Destroy()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	for (VkImageView view : LevelViews)
	{
		vkDestroyImageView(device, view, nullptr);
	}
	if (View) vkDestroyImageView(device, View, nullptr);
	if (Handle) vkDestroyImage(device, Handle, nullptr);
	if (Memory) vkFreeMemory(device, Memory, nullptr);
	*this = EnvMapVulkanComputeImage();
}

void EnvMapVulkanTemporalFilter::Create(const std::string& shaderName, VkFormat format)
{
	m_Format = format;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxAnisotropy = 1.0f;
	m_Sampler = H2M::VulkanShaderH2M::GetSampler(samplerInfo); // shared: not destroyed here

	m_Pipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get(shaderName));
	for (H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set : m_Sets)
	{
		set = m_Pipeline->GetShader()->CreateDescriptorSets();
	}
}

void EnvMapVulkanTemporalFilter::Destroy()
{
	for (uint32_t i = 0; i < 2; i++)
	{
		m_History[i].Destroy();
		m_HistoryInfo[i].Destroy();
		m_Sets[i] = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
	}
	if (m_Sampler)
	{
		m_Sampler = VK_NULL_HANDLE;
	}
	m_Pipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>();
	m_Width = m_Height = 0;
}

void EnvMapVulkanTemporalFilter::Resize(uint32_t width, uint32_t height)
{
	for (uint32_t i = 0; i < 2; i++)
	{
		m_History[i].Destroy();
		m_HistoryInfo[i].Destroy();
		m_History[i].Create(m_Format, width, height);
		m_HistoryInfo[i].Create(HistoryInfoFormat, width, height, 1, 0.0f);
	}
	m_Width = width;
	m_Height = height;
	m_Result = 0;
	m_HasHistory = false;
}

void EnvMapVulkanTemporalFilter::SetInputs(const VkDescriptorImageInfo& current, const VkDescriptorImageInfo& currentDepth, const VkDescriptorImageInfo& motion)
{
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_Pipeline->GetShader();
	for (uint32_t i = 0; i < 2; i++)
	{
		const VkDescriptorImageInfo history = m_History[1 - i].GetSampledInfo(m_Sampler);
		const VkDescriptorImageInfo historyInfo = m_HistoryInfo[1 - i].GetSampledInfo(m_Sampler);
		const VkDescriptorImageInfo result = m_History[i].GetStorageInfo();
		const VkDescriptorImageInfo resultInfo = m_HistoryInfo[i].GetStorageInfo();
		const char* names[7] = { "u_Current", "u_CurrentDepth", "u_Motion", "u_History", "u_HistoryInfo", "o_History", "o_HistoryInfo" };
		const VkDescriptorImageInfo* infos[7] = { &current, &currentDepth, &motion, &history, &historyInfo, &result, &resultInfo };
		std::array<VkWriteDescriptorSet, 7> writes;
		for (size_t w = 0; w < writes.size(); w++)
		{
			writes[w] = *shader->GetDescriptorSet(names[w]);
			writes[w].dstSet = m_Sets[i].DescriptorSets[0];
			writes[w].descriptorCount = 1;
			writes[w].pImageInfo = infos[w];
		}
		vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
	}
}

void EnvMapVulkanTemporalFilter::Record(VkCommandBuffer commandBuffer, float historyWeight, float depthTolerance)
{
	m_Result = 1 - m_Result;

	Push push;
	push.MaxFrames = 1.0f / std::max(1.0f - historyWeight, 0.01f);
	push.DepthTolerance = depthTolerance;
	push.NoHistory = m_HasHistory ? 0.0f : 1.0f;
	push.RangeMargin = 0.0f;
	m_HasHistory = true;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline->GetVulkanPipeline());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline->GetVulkanPipelineLayout(), 0, 1,
		m_Sets[m_Result].DescriptorSets.data(), 0, nullptr);
	vkCmdPushConstants(commandBuffer, m_Pipeline->GetVulkanPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
	vkCmdDispatch(commandBuffer, (m_Width + 7) / 8, (m_Height + 7) / 8, 1);
}
