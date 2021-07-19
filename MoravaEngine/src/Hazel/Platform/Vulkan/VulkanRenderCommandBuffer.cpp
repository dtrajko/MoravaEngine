#include "VulkanRenderCommandBuffer.h"


namespace Hazel {

	VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(uint32_t count, std::string debugName)
	{
	}

	VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(std::string debugName, bool swapchain)
	{
	}

	VulkanRenderCommandBuffer::~VulkanRenderCommandBuffer()
	{
	}

	void VulkanRenderCommandBuffer::Begin()
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::Begin - method not yet implemented!");
	}

	void VulkanRenderCommandBuffer::End()
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::End - method not yet implemented!");
	}

	void VulkanRenderCommandBuffer::Submit()
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::Submit - method not yet implemented!");
	}

	float VulkanRenderCommandBuffer::GetExecutionGPUTime(uint32_t frameIndex, uint32_t queryIndex) const
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::GetExecutionGPUTime - method not yet implemented!");

		return 0.0f;
	}

	const PipelineStatistics& VulkanRenderCommandBuffer::GetPipelineStatistics(uint32_t frameIndex) const
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::GetPipelineStatistics - method not yet implemented!");

		PipelineStatistics pipelineStatistics = {};
		return pipelineStatistics;
	}

	uint64_t VulkanRenderCommandBuffer::BeginTimestampQuery()
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::BeginTimestampQuery - method not yet implemented!");

		return uint64_t();
	}

	void VulkanRenderCommandBuffer::EndTimestampQuery(uint64_t queryID)
	{
		Log::GetLogger()->warn("VulkanRenderCommandBuffer::EndTimestampQuery - method not yet implemented!");
	}

}
