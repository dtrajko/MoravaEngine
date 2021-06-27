#include "DX11PixelShader.h"

#include "DX11Context.h"

#include <exception>


DX11PixelShader::DX11PixelShader(const wchar_t* pixelShaderPath)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	CompileDX11Shader(pixelShaderPath);

	HRESULT hr = dx11Device->CreatePixelShader(m_BytecodePointer, m_BytecodeLength, nullptr, &m_DX11PixelShader);
	if (FAILED(hr))
	{
		throw std::exception("DX11PixelShader initialization failed.");
	}

	// ReleaseCompiledDX11Shader();

	Log::GetLogger()->info("DX11PixelShader '{0}' has been successfully created!", Util::to_str(pixelShaderPath));
}

DX11PixelShader::~DX11PixelShader()
{
	// ReleaseCompiledDX11Shader();
	if (m_DX11PixelShader) m_DX11PixelShader->Release();

	Log::GetLogger()->info("DX11PixelShader destroyed!");
}

void DX11PixelShader::Bind()
{
	DX11Context::Get()->GetDX11DeviceContext()->PSSetShader(m_DX11PixelShader, nullptr, 0);
}

void DX11PixelShader::BindConstantBuffer(Hazel::Ref<DX11ConstantBuffer> constantBuffer)
{
	DX11Context::Get()->GetDX11DeviceContext()->PSSetConstantBuffers(0, 1, &constantBuffer->m_Buffer);
}

void DX11PixelShader::SetTextures(const std::vector<Hazel::Ref<DX11Texture2D>>& textures)
{
	size_t textureCount = textures.size();

	ID3D11ShaderResourceView* list_res[32];
	ID3D11SamplerState* list_sampler[32];

	for (unsigned int i = 0; i < textureCount; i++)
	{
		list_res[i] = textures[i]->m_ShaderResourceView;
		list_sampler[i] = textures[i]->m_SamplerState;
	}

	DX11Context::Get()->GetDX11DeviceContext()->PSSetShaderResources(0, (UINT)textureCount, list_res);
	DX11Context::Get()->GetDX11DeviceContext()->PSSetSamplers(0, (UINT)textureCount, list_sampler);
}

bool DX11PixelShader::CompileDX11Shader(const wchar_t* fileName)
{
	const char* entryPointName = "psmain";
	const char* entryPoint = "ps_5_0";

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

	Log::GetLogger()->info("DX11PixelShader '{0}' has been successfully compiled!", Util::to_str(fileName));

	return true;
}

void DX11PixelShader::ReleaseCompiledDX11Shader()
{
	if (m_Blob) m_Blob->Release();
}
