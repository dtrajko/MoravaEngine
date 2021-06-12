#include "DX11Texture2D.h"

#include "DX11Context.h"
#include "DX11Image.h"

#include <DirectXTex.h>


DX11Texture2D::DX11Texture2D()
{
}

DX11Texture2D::DX11Texture2D(const wchar_t* full_path)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetCurrentDevice()->GetDX11Device();

	DirectX::ScratchImage image_data;
	HRESULT res = DirectX::LoadFromWICFile(full_path, DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, image_data);

	if (SUCCEEDED(res))
	{
		res = DirectX::CreateTexture(dx11Device, image_data.GetImages(),
			image_data.GetImageCount(), image_data.GetMetadata(), &m_texture);

		if (FAILED(res)) throw std::exception("DX11Texture2D not created successfully.");

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

		res = dx11Device->CreateSamplerState(&sampler_desc, &m_sampler_state);
		if (FAILED(res)) throw std::exception("DX11Texture2D SamplerState not created successfully.");

		res = dx11Device->CreateShaderResourceView(m_texture, &desc, &m_shader_res_view);
		if (FAILED(res)) throw std::exception("DX11Texture2D ShaderResourceView not created successfully.");
	}
	else
	{
		throw std::exception("DX11Texture2D not created successfully.");
	}

	// Invalidate(); // do we need an Invalidate method?
}

DX11Texture2D::DX11Texture2D(const glm::vec2& size, DX11Texture2D::Type type)
{
	ID3D11Device* dx11Device = DX11Context::Get()->GetCurrentDevice()->GetDX11Device();

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

	auto hr = dx11Device->CreateTexture2D(&tex_desc, nullptr, (ID3D11Texture2D**)&m_texture);

	if (FAILED(hr))
	{
		throw std::exception("DX11Texture2D not created successfully (Texture2D).");
	}

	if (type == Normal || type == RenderTarget)
	{
		hr = dx11Device->CreateShaderResourceView(m_texture, NULL, &m_shader_res_view);
		if (FAILED(hr))
		{
			throw std::exception("DX11Texture2D not created successfully (ShaderResourceView).");
		}
	}

	if (type == RenderTarget)
	{
		hr = dx11Device->CreateRenderTargetView(m_texture, NULL, &m_render_target_view);
		if (FAILED(hr))
		{
			throw std::exception("DX11Texture2D not created successfully (RenderTargetView).");
		}
	}
	else if (type == DepthStencil)
	{
		hr = dx11Device->CreateDepthStencilView(m_texture, NULL, &m_depth_stencil_view);
		if (FAILED(hr))
		{
			throw std::exception("DX11Texture2D not created successfully (DepthStencilView).");
		}
	}

	m_type = type;
	m_size = size;

	// Invalidate(); // do we need an Invalidate method?
}

DX11Texture2D::DX11Texture2D(const std::string& path, bool srgb, Hazel::HazelTextureWrap wrap)
	: m_Path(path)
{
	Log::GetLogger()->error("This version of DX11Texture2D constructor is not used in DirecX 11!");

	// Invalidate();

	HZ_CORE_ASSERT(false);
}

DX11Texture2D::DX11Texture2D(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data, Hazel::HazelTextureWrap wrap)
	: m_Format(format)
{
	Log::GetLogger()->error("This version of DX11Texture2D constructor is not used in DirecX 11!");

	// Invalidate();

	HZ_CORE_ASSERT(false);
}

DX11Texture2D::DX11Texture2D(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, Hazel::HazelTextureWrap wrap)
{
	Log::GetLogger()->error("This version of DX11Texture2D constructor is not used in DirecX 11!");

	// Invalidate();

	HZ_CORE_ASSERT(false);
}

DX11Texture2D::~DX11Texture2D()
{
	if (m_render_target_view) m_render_target_view->Release();
	if (m_depth_stencil_view) m_depth_stencil_view->Release();
	if (m_shader_res_view) m_shader_res_view->Release();
	if (m_sampler_state) m_sampler_state->Release();
	if (m_texture) m_texture->Release();
}

void DX11Texture2D::Invalidate()
{
	auto device = DX11Context::GetCurrentDevice();
	// auto DX11Device = device->GetDX11Device();

	// TODO
}

void DX11Texture2D::Bind(uint32_t slot) const
{
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

Hazel::Buffer DX11Texture2D::GetWriteableBuffer()
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

Hazel::HazelImageFormat DX11Texture2D::GetFormat() const
{
	return Hazel::HazelImageFormat::None;
}

uint32_t DX11Texture2D::GetMipLevelCount() const
{
	return 0;
}

Hazel::RendererID DX11Texture2D::GetRendererID() const
{
	return 0;
}
