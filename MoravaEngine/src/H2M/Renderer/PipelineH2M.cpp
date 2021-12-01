#include "PipelineH2M.h"

#include "Hazel/Platform/OpenGL/OpenGLPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Platform/DX11/DX11Pipeline.h"
#include "Hazel/Renderer/RendererAPI.h"


namespace H2M
{

	H2M::RefH2M<PipelineH2M> PipelineH2M::Create(const PipelineSpecification& spec)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return H2M::RefH2M<Pipeline>();
			case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<H2M::OpenGLPipeline>::Create(spec);
			case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<H2M::VulkanPipeline>::Create(spec);
			case H2M::RendererAPITypeH2M::DX11:   return H2M::RefH2M<DX11Pipeline>::Create(spec);
		}

		H2M::Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<PipelineH2M>();
	}
}
