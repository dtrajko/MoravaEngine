#include "DX11VertexShader.h"

#include "DX11Context.h"

#include <exception>


DX11VertexShader::DX11VertexShader(const wchar_t* vertexShaderPath)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	CompileDX11Shader(vertexShaderPath);

	HRESULT hr = dx11Device->CreateVertexShader(m_BytecodePointer, m_BytecodeLength, nullptr, &m_DX11VertexShader);
	if (FAILED(hr))
	{
		throw std::exception("DX11VertexShader initialization failed.");
	}

	// ReleaseCompiledDX11Shader();
	Log::GetLogger()->info("DX11VertexShader '{0}' has been successfully created!", Util::to_str(vertexShaderPath));
}

DX11VertexShader::~DX11VertexShader()
{
	// ReleaseCompiledDX11Shader();
	if (m_DX11VertexShader) m_DX11VertexShader->Release();

	Log::GetLogger()->info("DX11VertexShader destroyed!");
}

void DX11VertexShader::Bind()
{
	DX11Context::Get()->GetDX11DeviceContext()->VSSetShader(m_DX11VertexShader, nullptr, 0);
}

void DX11VertexShader::BindConstantBuffer(Hazel::Ref<DX11ConstantBuffer> constantBuffer)
{
	DX11Context::Get()->GetDX11DeviceContext()->VSSetConstantBuffers(0, 1, &constantBuffer->m_Buffer);
}

void DX11VertexShader::SetTextures(const std::vector<Hazel::Ref<Hazel::HazelTexture>>& textures)
{
	size_t textureCount = textures.size();

	ID3D11ShaderResourceView* list_res[32];
	ID3D11SamplerState* list_sampler[32];

	for (unsigned int i = 0; i < textureCount; i++)
	{
		list_res[i] = textures[i].As<DX11Texture2D>()->m_ShaderResourceViewDX11;
		list_sampler[i] = textures[i].As<DX11Texture2D>()->m_SamplerStateDX11;
	}

	DX11Context::Get()->GetDX11DeviceContext()->VSSetShaderResources(0, (UINT)textureCount, list_res);
	DX11Context::Get()->GetDX11DeviceContext()->VSSetSamplers(0, (UINT)textureCount, list_sampler);
}

bool DX11VertexShader::CompileDX11Shader(const wchar_t* fileName)
{
	const char* entryPointName = "vsmain";
	const char* entryPoint = "vs_5_0";

	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = ::D3DCompileFromFile(fileName, nullptr, nullptr, entryPointName, entryPoint, 0, 0, &m_Blob, &errorBlob);

	if (FAILED(hr))
	{
		if (errorBlob)
		{
			errorBlob->Release();
		}
		return false;
	}

	m_BytecodePointer = m_Blob->GetBufferPointer();
	m_BytecodeLength = m_Blob->GetBufferSize();

	Log::GetLogger()->info("DX11VertexShader '{0}' has been successfully compiled!", Util::to_str(fileName));

	return true;
}

void DX11VertexShader::ReleaseCompiledDX11Shader()
{
	if (m_Blob) m_Blob->Release();
}
