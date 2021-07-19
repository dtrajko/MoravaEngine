#pragma once

#include "Hazel/Renderer/RenderCommandBuffer.h"

#include "Vulkan.h"


namespace Hazel {

	class VulkanRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		VulkanRenderCommandBuffer(uint32_t count = 0, std::string debugName = "");
		VulkanRenderCommandBuffer(std::string debugName, bool swapchain);
		~VulkanRenderCommandBuffer() override;

		virtual void Begin() override;
		virtual void End() override;
		virtual void Submit() override;

		virtual float GetExecutionGPUTime(uint32_t frameIndex, uint32_t queryIndex = 0) const override;
		virtual const PipelineStatistics& GetPipelineStatistics(uint32_t frameIndex) const override;

		virtual uint64_t BeginTimestampQuery() override;
		virtual void EndTimestampQuery(uint64_t queryID) override;

	};

}
