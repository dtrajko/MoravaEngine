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
