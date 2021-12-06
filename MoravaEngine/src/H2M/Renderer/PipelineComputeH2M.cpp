/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "PipelineComputeH2M.h"

#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"

namespace H2M
{

	RefH2M<PipelineComputeH2M> PipelineComputeH2M::Create(RefH2M<ShaderH2M> computeShader)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None: return RefH2M<PipelineComputeH2M>();
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanComputePipelineH2M>::Create(computeShader);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<PipelineComputeH2M>();
	}

}
