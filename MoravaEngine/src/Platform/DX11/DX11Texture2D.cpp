#include "DX11Texture2D.h"

#include "DX11Context.h"
#include "DX11Image.h"

#include "Core/Util.h"

#include <DirectXTex.h>


DX11Texture2D::DX11Texture2D()
{
}

DX11Texture2D::DX11Texture2D(const wchar_t* fullPath)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	DirectX::ScratchImage image_data;
	HRESULT res = DirectX::LoadFromWICFile(fullPath, DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, image_data);

	if (SUCCEEDED(res))
	{
		res = DirectX::CreateTexture(dx11Device, image_data.GetImages(),
			image_data.GetImageCount(), image_data.GetMetadata(), &m_TextureDX11);

		if (FAILED(res)) throw std::exception("DX11Texture2D not created successfully.");

		m_Path = Util::to_str(fullPath);
		m_Width = (uint32_t)image_data.GetMetadata().width;
		m_Height = (uint32_t)image_data.GetMetadata().height;
		m_Size = glm::vec2(m_Width, m_Height);

		D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = image_data.GetMetadata().format;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = (UINT)image_data.GetMetadata().mipLevels;
		desc.Texture2D.MostDetailedMip = 0;

		D3D11_SAMPLER_DESC sampler_desc = {};
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
		sampler_desc.MinLOD = 0;
		sampler_desc.MaxLOD = (FLOAT)image_data.GetMetadata().mipLevels;

		res = dx11Device->CreateSamplerState(&sampler_desc, &m_SamplerStateDX11);
		if (FAILED(res)) throw std::exception("DX11Texture2D SamplerState not created successfully.");

		res = dx11Device->CreateShaderResourceView(m_TextureDX11, &desc, &m_ShaderResourceViewDX11);
		if (FAILED(res)) throw std::exception("DX11Texture2D ShaderResourceView not created successfully.");
	}
	else
	{
		std::string message = "DX11Texture2D '" + Util::to_str(fullPath) + "' not created successfully.";
		Log::GetLogger()->warn(message);
		// throw std::exception(message.c_str());
	}

	Log::GetLogger()->info("DX11Texture2D '{0}' successfully loaded!", Util::to_str(fullPath));

	// Invalidate(); // do we need an Invalidate method?
}

DX11Texture2D::DX11Texture2D(const glm::vec2& size, DX11Texture2D::Type type)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	D3D11_TEXTURE2D_DESC tex_desc = {};
	tex_desc.Width = (UINT)size.x;
	tex_desc.Height = (UINT)size.y;

	if (type == Normal)
	{
		tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	}
	else if (type == RenderTarget)
	{
		tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	}
	else if (type == DepthStencil)
	{
		tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	}

	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.MipLevels = 1;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.MiscFlags = 0;
	tex_desc.ArraySize = 1;
	tex_desc.CPUAccessFlags = 0;

	auto hr = dx11Device->CreateTexture2D(&tex_desc, nullptr, (ID3D11Texture2D**)&m_TextureDX11);
	if (FAILED(hr))
	{
		throw std::exception("DX11Texture2D not created successfully.");
	}
	else if (SUCCEEDED(hr))
	{
		Log::GetLogger()->info("DX11Texture2D successfully created!");
	}

	if (type == Normal || type == RenderTarget)
	{
		hr = dx11Device->CreateShaderResourceView(m_TextureDX11, NULL, &m_ShaderResourceViewDX11);
		if (FAILED(hr))
		{
			throw std::exception("DX11Texture2D not created successfully (ShaderResourceView).");
		}
		else if (SUCCEEDED(hr))
		{
			Log::GetLogger()->info("DX11Texture2D: ShaderResourceView successfully created!");
		}
	}

	if (type == RenderTarget)
	{
		hr = dx11Device->CreateRenderTargetView(m_TextureDX11, NULL, &m_RenderTargetViewDX11);
		if (FAILED(hr))
		{
			throw std::exception("DX11Texture2D not created successfully (RenderTargetView).");
		}
		else if (SUCCEEDED(hr))
		{
			Log::GetLogger()->info("DX11Texture2D: RenderTargetView successfully created!");
		}
	}
	else if (type == DepthStencil)
	{
		hr = dx11Device->CreateDepthStencilView(m_TextureDX11, NULL, &m_DepthStencilViewDX11);
		if (FAILED(hr))
		{
			throw std::exception("DX11Texture2D not created successfully (DepthStencilView).");
		}
		else if (SUCCEEDED(hr))
		{
			Log::GetLogger()->info("DX11Texture2D: DepthStencilView successfully created!");
		}
	}

	m_Type = type;
	m_Size = size;

	Log::GetLogger()->info("DX11Texture2D successfully created!");

	// Invalidate(); // do we need an Invalidate method?
}

DX11Texture2D::DX11Texture2D(const std::string& path, bool srgb, H2M::TextureWrapH2M wrap)
	: m_Path(path)
{
	Log::GetLogger()->error("This version of DX11Texture2D constructor is not used in DirectX 11!");

	// Invalidate();

	H2M_CORE_ASSERT(false);
}

DX11Texture2D::DX11Texture2D(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, const void* data, H2M::TextureWrapH2M wrap) :
	DX11Texture2D(glm::vec2(width, height), DX11Texture2D::Type::Normal)
{
	m_Format = format;

	// Log::GetLogger()->error("This version of DX11Texture2D constructor is not used in DirectX 11!");
	// Invalidate();
	// H2M_CORE_ASSERT(false);
}

DX11Texture2D::DX11Texture2D(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, H2M::TextureWrapH2M wrap)
{
	Log::GetLogger()->error("This version of DX11Texture2D constructor is not used in DirectX 11!");

	// Invalidate();

	H2M_CORE_ASSERT(false);
}

DX11Texture2D::~DX11Texture2D()
{
	Release();

	if (m_RenderTargetViewDX11) m_RenderTargetViewDX11->Release();
	if (m_DepthStencilViewDX11) m_DepthStencilViewDX11->Release();
	if (m_ShaderResourceViewDX11) m_ShaderResourceViewDX11->Release();
	if (m_SamplerStateDX11) m_SamplerStateDX11->Release();
	if (m_TextureDX11) m_TextureDX11->Release();
}

void DX11Texture2D::Release()
{
}

void DX11Texture2D::Invalidate()
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetDX11Device();

	Release(); // release resources

	// TODO: Re-create resources
}

void DX11Texture2D::Bind(uint32_t slot) const
{
}

H2M::RefH2M<H2M::Image2D_H2M> DX11Texture2D::GetImage() const
{
	return H2M::RefH2M<H2M::Image2D_H2M>();
}

void DX11Texture2D::Lock()
{
}

void DX11Texture2D::Unlock()
{
}

void DX11Texture2D::Resize(uint32_t width, uint32_t height)
{
}

H2M::BufferH2M DX11Texture2D::GetWriteableBuffer()
{
	return m_ImageData;
}

bool DX11Texture2D::Loaded() const
{
	return true;
}

const std::string& DX11Texture2D::GetPath() const
{
	return m_Path;
}

H2M::ImageFormatH2M DX11Texture2D::GetFormat() const
{
	return H2M::ImageFormatH2M::None;
}

uint32_t DX11Texture2D::GetMipLevelCount() const
{
	return 0;
}

std::pair<uint32_t, uint32_t> DX11Texture2D::GetMipSize(uint32_t mip) const
{
	Log::GetLogger()->error("DX11Texture2D::GetMipSize({0}) - method not implemented!", mip);
	return std::pair<uint32_t, uint32_t>();
}

uint64_t DX11Texture2D::GetHash() const
{
	return uint64_t();
}
