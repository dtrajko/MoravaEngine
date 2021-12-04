#include "PipelineH2M.h"

#include "H2M/Platform/OpenGL/OpenGLPipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Platform/DX11/DX11Pipeline.h"


namespace H2M
{

	RefH2M<PipelineH2M> PipelineH2M::Create(const PipelineSpecificationH2M& spec)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   return RefH2M<PipelineH2M>();
			case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLPipelineH2M>::Create(spec);
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanPipelineH2M>::Create(spec);
			case RendererAPITypeH2M::DX11:   return RefH2M<DX11Pipeline>::Create(spec);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<PipelineH2M>();
	}
}
