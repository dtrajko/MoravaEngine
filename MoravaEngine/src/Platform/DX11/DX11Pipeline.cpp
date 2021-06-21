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
	if (m_InputLayout) m_InputLayout->Release();
}

void DX11Pipeline::Invalidate()
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		// SEMANTIC NAME - SEMANTIC INDEX - FORMAT - INPUT SLOT - ALIGNED BYTE OFFSET - INPUT SLOT CLASS - INSTANCE DATA STEP RATE
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	UINT numElements = ARRAYSIZE(layout);

	Hazel::Ref<DX11Shader> dx11Shader = m_Specification.Shader.As<DX11Shader>();

	const void* shaderBytecodePointer = dx11Shader->GetVertexShader()->GetBytecodePointer();
	size_t shaderBytecodeLength = dx11Shader->GetVertexShader()->GetBytecodeLength();

	HRESULT hr = dx11Device->CreateInputLayout(layout, numElements, shaderBytecodePointer, shaderBytecodeLength, &m_InputLayout);
	if (FAILED(hr))
	{
		throw std::exception("DX11Pipeline: CreateInputLayout failed.");
	}

	Log::GetLogger()->info("DX11Pipeline: InputLayout successfully created!");


	// Ref<DX11Pipeline> instance = this;
	// HazelRenderer::Submit([instance]() mutable {});
	{
		// Hazel::Ref<DX11Shader> DX11Shader = Hazel::Ref<DX11Shader>(m_Specification.Shader);
		// Hazel::Ref<DX11Framebuffer> framebuffer = m_Specification.RenderPass->GetSpecification().TargetFramebuffer.As<DX11Framebuffer>();

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
	DX11Context::Get()->GetDX11DeviceContext()->IASetInputLayout(m_InputLayout);
}
