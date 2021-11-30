#include "PipelineH2M.h"

#include "Hazel/Platform/OpenGL/OpenGLPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Platform/DX11/DX11Pipeline.h"
#include "Hazel/Renderer/RendererAPI.h"


namespace H2M
{

	H2M::Ref<PipelineH2M> PipelineH2M::Create(const PipelineSpecification& spec)
	{
		switch (H2M::RendererAPIH2M::Current())
		{
			case H2M::RendererAPIH2MType::None:   return H2M::Ref<Pipeline>();
			case H2M::RendererAPIH2MType::OpenGL: return H2M::Ref<H2M::OpenGLPipeline>::Create(spec);
			case H2M::RendererAPIH2MType::Vulkan: return H2M::Ref<H2M::VulkanPipeline>::Create(spec);
			case H2M::RendererAPIH2MType::DX11:   return H2M::Ref<DX11Pipeline>::Create(spec);
		}

		H2M::Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<PipelineH2M>();
	}
}
