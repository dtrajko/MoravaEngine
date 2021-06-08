#include "DX11Pipeline.h"

#include "DX11Context.h"
#include "DX11Framebuffer.h"
#include "Hazel/Renderer/HazelRenderer.h"


DX11Pipeline::DX11Pipeline(const Hazel::PipelineSpecification& spec)
	: m_Specification(spec)
{
	Invalidate();
}

DX11Pipeline::~DX11Pipeline()
{
	// TODO: delete pipeline
}

void DX11Pipeline::Invalidate()
{
	//	Ref<DX11Pipeline> instance = this;
	//	HazelRenderer::Submit([instance]() mutable
	//	{
	//	});
	{
		// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();

		HZ_CORE_ASSERT(m_Specification.Shader);
		// Hazel::Ref<DX11Shader> DX11Shader = Hazel::Ref<DX11Shader>(m_Specification.Shader);
		Hazel::Ref<DX11Framebuffer> framebuffer = m_Specification.RenderPass->GetSpecification().TargetFramebuffer.As<DX11Framebuffer>();

		//////////////////////////////////////////////////////////////////////
		// Push Constants
		//////////////////////////////////////////////////////////////////////


		// TODO: CreateGraphicsPipelines

		// Shader modules are no longer needed once the graphics pipeline has been created
		// TODO: DestroyShaderModule(device, shaderStages[0].module, nullptr); // ???
		// TODO: DestroyShaderModule(device, shaderStages[1].module, nullptr); // ???
	}
}

void DX11Pipeline::Bind()
{
}
