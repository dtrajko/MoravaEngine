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

	return true;
}

void DX11PixelShader::ReleaseCompiledDX11Shader()
{
	if (m_Blob) m_Blob->Release();
}
