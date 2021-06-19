#include "Pipeline.h"

#include "Hazel/Platform/OpenGL/OpenGLPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Platform/DX11/DX11Pipeline.h"


namespace Hazel {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:   return Ref<Pipeline>();
			case RendererAPIType::OpenGL: return Ref<OpenGLPipeline>::Create(spec);
			case RendererAPIType::Vulkan: return Ref<VulkanPipeline>::Create(spec);
			case RendererAPIType::DX11:   return Ref<DX11Pipeline>::Create(spec);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<Pipeline>();
	}
}
