#include "DX11VertexShader.h"

#include "DX11Context.h"

#include <exception>


DX11VertexShader::DX11VertexShader(const wchar_t* vertexShaderPath)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	CompileDX11Shader(vertexShaderPath);

	HRESULT hr = dx11Device->CreateVertexShader(m_ShaderBytecodePointer, m_BytecodeLength, nullptr, &m_DX11VertexShader);
	if (FAILED(hr))
	{
		throw std::exception("DX11VertexShader initialization failed.");
	}

	ReleaseCompiledDX11Shader();

	Log::GetLogger()->info("DX11VertexShader '{0}' has been successfully created!", Util::to_str(vertexShaderPath));
}

DX11VertexShader::~DX11VertexShader()
{
	m_DX11VertexShader->Release();
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

	m_ShaderBytecodePointer = m_Blob->GetBufferPointer();
	m_BytecodeLength = m_Blob->GetBufferSize();

	return true;
}

void DX11VertexShader::ReleaseCompiledDX11Shader()
{
	if (m_Blob) m_Blob->Release();
}
