/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanRenderCommandBufferH2M.h"

#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Renderer/RendererH2M.h"


namespace H2M {

	VulkanRenderCommandBufferH2M::VulkanRenderCommandBufferH2M(uint32_t count, std::string debugName)
		: m_DebugName(std::move(debugName))
	{
		auto device = VulkanContextH2M::GetCurrentDevice();
		uint32_t framesInFlight = RendererH2M::GetConfig().FramesInFlight;

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT_H2M(vkCreateCommandPool(device->GetVulkanDevice(), &cmdPoolInfo, nullptr, &m_CommandPool));

		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = m_CommandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		if (count == 0)
			count = framesInFlight;
		commandBufferAllocateInfo.commandBufferCount = count;
		m_CommandBuffers.resize(count);
		VK_CHECK_RESULT_H2M(vkAllocateCommandBuffers(device->GetVulkanDevice(), &commandBufferAllocateInfo, m_CommandBuffers.data()));

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		m_WaitFences.resize(framesInFlight);
		for (auto& fence : m_WaitFences)
			VK_CHECK_RESULT_H2M(vkCreateFence(device->GetVulkanDevice(), &fenceCreateInfo, nullptr, &fence));

		VkQueryPoolCreateInfo queryPoolCreateInfo = {};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.pNext = nullptr;

		// Timestamp queries
		const uint32_t maxUserQueries = 10;
		m_TimestampQueryCount = 2 + 2 * maxUserQueries;

		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = m_TimestampQueryCount;
		m_TimestampQueryPools.resize(framesInFlight);
		for (auto& timestampQueryPool : m_TimestampQueryPools)
			VK_CHECK_RESULT_H2M(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &timestampQueryPool));

		m_TimestampQueryResults.resize(framesInFlight);
		for (auto& timestampQueryResults : m_TimestampQueryResults)
			timestampQueryResults.resize(m_TimestampQueryCount);

		m_ExecutionGPUTimes.resize(framesInFlight);
		for (auto& executionGPUTimes : m_ExecutionGPUTimes)
			executionGPUTimes.resize(m_TimestampQueryCount / 2);

		// Pipeline statistics queries
		m_PipelineQueryCount = 7;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		queryPoolCreateInfo.queryCount = m_PipelineQueryCount;
		queryPoolCreateInfo.pipelineStatistics =
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

		m_PipelineStatisticsQueryPools.resize(framesInFlight);
		for (auto& pipelineStatisticsQueryPools : m_PipelineStatisticsQueryPools)
			VK_CHECK_RESULT_H2M(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPools));

		m_PipelineStatisticsQueryResults.resize(framesInFlight);
	}

	VulkanRenderCommandBufferH2M::VulkanRenderCommandBufferH2M(std::string debugName, bool swapchain)
		: m_DebugName(std::move(debugName)), m_OwnedBySwapChain(true)
	{
		auto device = VulkanContextH2M::GetCurrentDevice();
		uint32_t framesInFlight = RendererH2M::GetConfig().FramesInFlight;

		m_CommandBuffers.resize(framesInFlight);
		VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();
		for (uint32_t frame = 0; frame < framesInFlight; frame++)
			m_CommandBuffers[frame] = swapChain.GetDrawCommandBuffer(frame);

		VkQueryPoolCreateInfo queryPoolCreateInfo = {};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.pNext = nullptr;

		// Timestamp queries
		const uint32_t maxUserQueries = 10;
		m_TimestampQueryCount = 2 + 2 * maxUserQueries;

		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = m_TimestampQueryCount;
		m_TimestampQueryPools.resize(framesInFlight);
		for (auto& timestampQueryPool : m_TimestampQueryPools)
			VK_CHECK_RESULT_H2M(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &timestampQueryPool));

		m_TimestampQueryResults.resize(framesInFlight);
		for (auto& timestampQueryResults : m_TimestampQueryResults)
			timestampQueryResults.resize(m_TimestampQueryCount);

		m_ExecutionGPUTimes.resize(framesInFlight);
		for (auto& executionGPUTimes : m_ExecutionGPUTimes)
			executionGPUTimes.resize(m_TimestampQueryCount / 2);

		// Pipeline statistics queries
		m_PipelineQueryCount = 7;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		queryPoolCreateInfo.queryCount = m_PipelineQueryCount;
		queryPoolCreateInfo.pipelineStatistics =
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

		m_PipelineStatisticsQueryPools.resize(framesInFlight);
		for (auto& pipelineStatisticsQueryPools : m_PipelineStatisticsQueryPools)
			VK_CHECK_RESULT_H2M(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPools));

		m_PipelineStatisticsQueryResults.resize(framesInFlight);
	}

	VulkanRenderCommandBufferH2M::~VulkanRenderCommandBufferH2M()
	{
		if (m_OwnedBySwapChain) return;

		VkCommandPool commandPool = m_CommandPool;
		// RendererH2M::SubmitResourceFree([commandPool]() {});
		{
			auto device = VulkanContextH2M::GetCurrentDevice();
			vkDestroyCommandPool(device->GetVulkanDevice(), commandPool, nullptr);
		}
	}

	void VulkanRenderCommandBufferH2M::Begin()
	{
		m_TimestampNextAvailableQuery = 2;

		// RefH2M<VulkanRenderCommandBuffer> instance = this;
		// RendererH2M::Submit([instance]() mutable {});
		{
			uint32_t frameIndex = RendererH2M::GetCurrentFrameIndex();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cmdBufInfo.pNext = nullptr;

			VkCommandBuffer commandBuffer = m_CommandBuffers[frameIndex];
			VK_CHECK_RESULT_H2M(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

			// Timestamp query
			vkCmdResetQueryPool(commandBuffer, m_TimestampQueryPools[frameIndex], 0, m_TimestampQueryCount);
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_TimestampQueryPools[frameIndex], 0);

			// Pipeline stats query
			vkCmdResetQueryPool(commandBuffer, m_PipelineStatisticsQueryPools[frameIndex], 0, m_PipelineQueryCount);
			vkCmdBeginQuery(commandBuffer, m_PipelineStatisticsQueryPools[frameIndex], 0, 0);
		}
	}

	void VulkanRenderCommandBufferH2M::End()
	{
		// RefH2M<VulkanRenderCommandBuffer> instance = this;
		// RendererH2M::Submit([instance]() {});
		{
			uint32_t frameIndex = RendererH2M::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = m_CommandBuffers[frameIndex];
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_TimestampQueryPools[frameIndex], 1);
			vkCmdEndQuery(commandBuffer, m_PipelineStatisticsQueryPools[frameIndex], 0);
			VK_CHECK_RESULT_H2M(vkEndCommandBuffer(commandBuffer));
		}
	}

	void VulkanRenderCommandBufferH2M::Submit()
	{
		if (m_OwnedBySwapChain) return;

		// RefH2M<VulkanRenderCommandBuffer> instance = this;
		// RendererH2M::Submit([instance]() mutable {});
		{
			auto device = VulkanContextH2M::GetCurrentDevice();

			uint32_t frameIndex = RendererH2M::GetCurrentFrameIndex();

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			VkCommandBuffer commandBuffer = m_CommandBuffers[frameIndex];
			submitInfo.pCommandBuffers = &commandBuffer;

			VK_CHECK_RESULT_H2M(vkWaitForFences(device->GetVulkanDevice(), 1, &m_WaitFences[frameIndex], VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT_H2M(vkResetFences(device->GetVulkanDevice(), 1, &m_WaitFences[frameIndex]));
			VK_CHECK_RESULT_H2M(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, m_WaitFences[frameIndex]));

			// Retrieve timestamp query results
			vkGetQueryPoolResults(device->GetVulkanDevice(), m_TimestampQueryPools[frameIndex], 0, m_TimestampNextAvailableQuery,
				m_TimestampNextAvailableQuery * sizeof(uint64_t), m_TimestampQueryResults[frameIndex].data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

			for (uint32_t i = 0; i < m_TimestampNextAvailableQuery; i += 2)
			{
				uint64_t startTime = m_TimestampQueryResults[frameIndex][i];
				uint64_t endTime = m_TimestampQueryResults[frameIndex][i + 1];
				float nsTime = endTime > startTime ? (endTime - startTime) * device->GetPhysicalDevice()->GetLimits().timestampPeriod : 0.0f;
				m_ExecutionGPUTimes[frameIndex][i / 2] = nsTime * 0.000001f; // Time in ms
			}

			// Retrieve pipeline stats results
			vkGetQueryPoolResults(device->GetVulkanDevice(), m_PipelineStatisticsQueryPools[frameIndex], 0, 1,
				sizeof(PipelineStatisticsH2M), &m_PipelineStatisticsQueryResults[frameIndex], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
		}
	}

	const PipelineStatisticsH2M& VulkanRenderCommandBufferH2M::GetPipelineStatistics(uint32_t frameIndex) const
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::GetPipelineStatistics - method not yet implemented!");

		PipelineStatisticsH2M pipelineStatistics = {};
		return pipelineStatistics;
	}

	uint64_t VulkanRenderCommandBufferH2M::BeginTimestampQuery()
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::BeginTimestampQuery - method not yet implemented!");

		return uint64_t();
	}

	void VulkanRenderCommandBufferH2M::EndTimestampQuery(uint64_t queryID)
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::EndTimestampQuery - method not yet implemented!");
	}

}
