/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanComputePipelineH2M.h"

#include "H2M/Core/TimerH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanDiagnosticsH2M.h"
#include "H2M/Renderer/RendererH2M.h"


namespace H2M
{

	static VkFence s_ComputeFence = nullptr;

	VulkanComputePipelineH2M::VulkanComputePipelineH2M(RefH2M<ShaderH2M> computeShader)
		: m_Shader(computeShader.As<VulkanShaderH2M>())
	{
		// Ref<VulkanComputePipeline> instance = this;
		// HazelRenderer::Submit([instance]() mutable { instance->CreatePipeline(); });

		CreatePipeline();
	}

	VulkanComputePipelineH2M::~VulkanComputePipelineH2M()
	{
	}

	void VulkanComputePipelineH2M::CreatePipeline()
	{
		VkDevice device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

		// TODO: Abstract into some sort of compute pipeline

		auto descriptorSetLayouts = m_Shader->GetAllDescriptorSetLayouts();
		auto descriptorSet = m_Shader->CreateDescriptorSets();

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

		const auto& pushConstantRanges = m_Shader->GetPushConstantRanges();
		std::vector<VkPushConstantRange> vulkanPushConstantRanges(pushConstantRanges.size());
		if (pushConstantRanges.size())
		{
			// TODO: should come from shader
			for (uint32_t i = 0; i < pushConstantRanges.size(); i++)
			{
				const auto& pushConstantRange = pushConstantRanges[i];
				auto& vulkanPushConstantRange = vulkanPushConstantRanges[i];

				vulkanPushConstantRange.stageFlags = pushConstantRange.ShaderStage;
				vulkanPushConstantRange.offset = pushConstantRange.Offset;
				vulkanPushConstantRange.size = pushConstantRange.Size;
			}

			pipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)vulkanPushConstantRanges.size();
			pipelineLayoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();
		}

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_ComputePipelineLayout));

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = m_ComputePipelineLayout;
		computePipelineCreateInfo.flags = 0;
		const auto& shaderStages = m_Shader->GetPipelineShaderStageCreateInfos();
		computePipelineCreateInfo.stage = shaderStages[0];

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	
		VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
		VK_CHECK_RESULT(vkCreateComputePipelines(device, m_PipelineCache, 1, &computePipelineCreateInfo, nullptr, &m_ComputePipeline));
	}

	void VulkanComputePipelineH2M::Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		VkDevice device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

		VkQueue computeQueue = VulkanContextH2M::GetCurrentDevice()->GetComputeQueue();
		//vkQueueWaitIdle(computeQueue); // TODO: don't

		VkCommandBuffer computeCommandBuffer = VulkanContextH2M::GetCurrentDevice()->GetCommandBuffer(true, true);

		Utils::SetVulkanCheckpoint(computeCommandBuffer, "VulkanComputePipeline::Execute");

		vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
		for (uint32_t i = 0; i < descriptorSetCount; i++)
		{
			vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, &descriptorSets[i], 0, 0);
			vkCmdDispatch(computeCommandBuffer, groupCountX, groupCountY, groupCountZ);
		}

		vkEndCommandBuffer(computeCommandBuffer);

		if (!s_ComputeFence)
		{
			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &s_ComputeFence));
		}

		// Make sure previous compute shader in pipeline has completed (TODO: this shouldn't be needed for all cases)
		vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &s_ComputeFence);

		VkSubmitInfo computeSubmitInfo{};
		computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &computeCommandBuffer;
		VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, s_ComputeFence));

		// Wait for execution of compute shader to complete
		// Currently this is here for "safety"
		{
			TimerH2M timer;
			vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
			Log::GetLogger()->trace("Compute shader execution took {0} ms", timer.ElapsedMillis());
		}
	}

	void VulkanComputePipelineH2M::Begin()
	{
		H2M_CORE_ASSERT(!m_ActiveComputeCommandBuffer);

		m_ActiveComputeCommandBuffer = VulkanContextH2M::GetCurrentDevice()->GetCommandBuffer(true, true);
		vkCmdBindPipeline(m_ActiveComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
	}

	void VulkanComputePipelineH2M::Dispatch(VkDescriptorSet descriptorSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		H2M_CORE_ASSERT(m_ActiveComputeCommandBuffer);

		vkCmdBindDescriptorSets(m_ActiveComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, &descriptorSet, 0, 0);
		vkCmdDispatch(m_ActiveComputeCommandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanComputePipelineH2M::End()
	{
		H2M_CORE_ASSERT(m_ActiveComputeCommandBuffer);

		VkDevice device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
		VkQueue computeQueue = VulkanContextH2M::GetCurrentDevice()->GetComputeQueue();

		vkEndCommandBuffer(m_ActiveComputeCommandBuffer);

		if (!s_ComputeFence)
		{
			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &s_ComputeFence));
		}
		vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &s_ComputeFence);

		VkSubmitInfo computeSubmitInfo{};
		computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &m_ActiveComputeCommandBuffer;
		VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, s_ComputeFence));

		// Wait for execution of compute shader to complete
		// Currently this is here for "safety"
		{
			TimerH2M timer;
			vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
			Log::GetLogger()->trace("Compute shader execution took {0} ms", timer.ElapsedMillis());
		}

		m_ActiveComputeCommandBuffer = nullptr;
	}

	void VulkanComputePipelineH2M::SetPushConstants(const void* data, uint32_t size)
	{
		vkCmdPushConstants(m_ActiveComputeCommandBuffer, m_ComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, size, data);

	}

}
