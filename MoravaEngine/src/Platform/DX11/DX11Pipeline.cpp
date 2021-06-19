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
	// It makes sense to move vertex buffer layout from DX11VerexBuffer to here

	auto dx11Device = DX11Context::Get()->GetDX11Device();

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		// SEMANTIC NAME - SEMANTIC INDEX - FORMAT - INPUT SLOT - ALIGNED BYTE OFFSET - INPUT SLOT CLASS - INSTANCE DATA STEP RATE
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	UINT size_layout = ARRAYSIZE(layout);

	const void* pShaderBytecodeWithInputSignature = m_Specification.Shader.As<DX11Shader>()->GetBytecodeWithInputSignature(); // TODO: implement in DX11Shader class
	size_t bytecodeLength = m_Specification.Shader.As<DX11Shader>()->GetBytecodeLength(); // TODO: implement in DX11Shader class

	/**** TODO: implement missing DX11Shader methods
	HRESULT hr = dx11Device->CreateInputLayout(layout, size_layout, pShaderBytecodeWithInputSignature, bytecodeLength, &m_layout);
	if (FAILED(hr))
	{
		throw std::exception("DX11Pipeline: CreateInputLayout failed.");
	}

	Log::GetLogger()->info("DX11Pipeline: InputLayout successfully created!");
	****/

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
