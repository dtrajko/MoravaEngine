#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/RenderCommandBufferH2M.h"

#include "VulkanH2M.h"


namespace H2M {

	class VulkanRenderCommandBufferH2M : public RenderCommandBufferH2M
	{
	public:
		VulkanRenderCommandBufferH2M(uint32_t count = 0, std::string debugName = "");
		VulkanRenderCommandBufferH2M(std::string debugName, bool swapchain);
		~VulkanRenderCommandBufferH2M() override;

		virtual void Begin() override;
		virtual void End() override;
		virtual void Submit() override;

		virtual float GetExecutionGPUTime(uint32_t frameIndex, uint32_t queryIndex = 0) const override
		{
			if (queryIndex / 2 >= m_TimestampNextAvailableQuery / 2)
				return 0.0f;

			return m_ExecutionGPUTimes[frameIndex][queryIndex / 2];
		}

		virtual const PipelineStatistics& GetPipelineStatistics(uint32_t frameIndex) const override;

		virtual uint64_t BeginTimestampQuery() override;
		virtual void EndTimestampQuery(uint64_t queryID) override;

		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
		{
			HZ_CORE_ASSERT(frameIndex < m_CommandBuffers.size());
			return m_CommandBuffers[frameIndex];
		}

	private:
		std::string m_DebugName;
		VkCommandPool m_CommandPool = nullptr;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		std::vector<VkFence> m_WaitFences;

		bool m_OwnedBySwapChain = false;

		uint32_t m_TimestampQueryCount = 0;
		uint32_t m_TimestampNextAvailableQuery = 2;
		std::vector<VkQueryPool> m_TimestampQueryPools;
		std::vector<VkQueryPool> m_PipelineStatisticsQueryPools;
		std::vector<std::vector<uint64_t>> m_TimestampQueryResults;
		std::vector<std::vector<float>> m_ExecutionGPUTimes;

		uint32_t m_PipelineQueryCount = 0;
		std::vector<PipelineStatistics> m_PipelineStatisticsQueryResults;
	};

}
