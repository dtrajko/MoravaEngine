#include "Pipeline.h"

#include "Hazel/Platform/OpenGL/OpenGLPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"


namespace Hazel {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return Ref<Pipeline>();
			case RendererAPIType::OpenGL:  return Ref<OpenGLPipeline>::Create(spec);
			case RendererAPIType::Vulkan:  return Ref<VulkanPipeline>::Create(spec);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<Pipeline>();
	}
}
