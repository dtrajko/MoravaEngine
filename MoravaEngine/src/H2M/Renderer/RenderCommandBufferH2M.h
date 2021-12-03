#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/PipelineH2M.h"

#include <string>


namespace H2M
{

	struct PipelineStatisticsH2M
	{
		uint64_t InputAssemblyVertices = 0;
		uint64_t InputAssemblyPrimitives = 0;
		uint64_t VertexShaderInvocations = 0;
		uint64_t ClippingInvocations = 0;
		uint64_t ClippingPrimitives = 0;
		uint64_t FragmentShaderInvocations = 0;
		uint64_t ComputeShaderInvocations = 0;

		// TODO(Yan): tesselation shader stats when we have them
	};

	class RenderCommandBufferH2M : public RefCountedH2M
	{
	public:
		virtual ~RenderCommandBufferH2M() {}

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Submit() = 0;

		virtual float GetExecutionGPUTime(uint32_t frameIndex, uint32_t queryIndex = 0) const = 0;
		virtual const PipelineStatistics& GetPipelineStatistics(uint32_t frameIndex) const = 0;

		virtual uint64_t BeginTimestampQuery() = 0;
		virtual void EndTimestampQuery(uint64_t queryID) = 0;

		static RefH2M<RenderCommandBufferH2M> Create(uint32_t count = 0, const std::string& debugName = "");
		static RefH2M<RenderCommandBufferH2M> CreateFromSwapChain(const std::string& debugName = "");

	};

}
