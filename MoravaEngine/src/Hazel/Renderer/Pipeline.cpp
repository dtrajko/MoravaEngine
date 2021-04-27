#include "Pipeline.h"

#include "../Platform/OpenGL/OpenGLPipeline.h"


namespace Hazel {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPI::RendererAPIType::None:    return Ref<Pipeline>();
			case RendererAPI::RendererAPIType::OpenGL:  return Ref<OpenGLPipeline>::Create(spec);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<Pipeline>();
	}
}
