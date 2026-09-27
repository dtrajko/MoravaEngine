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

void DX11VertexShader::BindConstantBuffer(H2M::RefH2M<DX11ConstantBuffer> constantBuffer)
{
	DX11Context::Get()->GetDX11DeviceContext()->VSSetConstantBuffers(0, 1, &constantBuffer->m_Buffer);
}

void DX11VertexShader::SetTextures(const std::vector<H2M::RefH2M<H2M::TextureH2M>>& textures)
{
	constexpr size_t MaxTextures = 32;

	size_t textureCount = textures.size();

	if (textureCount > MaxTextures)
	{
		throw std::runtime_error("DX11VertexShader::SetTextures - Too many textures bound");
	}

	ID3D11ShaderResourceView* list_res[MaxTextures] = {};
	ID3D11SamplerState* list_sampler[MaxTextures] = {};

	for (size_t i = 0; i < textureCount; i++)
	{
		auto texture = textures[i].As<DX11Texture2D>();

		if (!texture)
		{
			throw std::runtime_error("DX11VertexShader::SetTextures - Invalid DX11Texture2D");
		}

		if (!texture->m_ShaderResourceViewDX11)
		{
			throw std::runtime_error("DX11VertexShader::SetTextures - Missing ShaderResourceView");
		}

		if (!texture->m_SamplerStateDX11)
		{
			throw std::runtime_error("DX11VertexShader::SetTextures - Missing SamplerState");
		}

		list_res[i] = texture->m_ShaderResourceViewDX11;
		list_sampler[i] = texture->m_SamplerStateDX11;
	}

	auto context = DX11Context::Get()->GetDX11DeviceContext();

	if (!context)
	{
		throw std::runtime_error("DX11VertexShader::SetTextures - Invalid DeviceContext");
	}

	for (size_t i = 0; i < textureCount; ++i)
	{
		auto srv = list_res[i];

		if (!srv)
			continue;

		ULONG refs = srv->AddRef();
		srv->Release();

		std::cout << "SRV[" << i << "] RefCount = " << refs << std::endl;
	}

	context->VSSetShaderResources(
		0,
		static_cast<UINT>(textureCount),
		list_res
	);

	for (size_t i = 0; i < textureCount; i++)
	{
		auto sampler = list_sampler[i];

		assert(sampler != nullptr);

		std::cout << "Sampler[" << i << "] = " << sampler << std::endl;
	}

	context->VSSetSamplers(
		0,
		static_cast<UINT>(textureCount),
		list_sampler
	);
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
