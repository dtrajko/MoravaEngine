#include "PipelineCompute.h"

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Platform/Vulkan/VulkanComputePipeline.h"

namespace Hazel {

	Ref<PipelineCompute> PipelineCompute::Create(Ref<HazelShader> computeShader)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return Ref<PipelineCompute>();
			case RendererAPIType::Vulkan: return Ref<VulkanComputePipeline>::Create(computeShader);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<PipelineCompute>();
	}

}
