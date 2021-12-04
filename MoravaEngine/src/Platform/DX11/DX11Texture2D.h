#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/TextureH2M.h"

#include "DX11.h"

#include "DX11Image.h"


class DX11Texture2D : public H2M::Texture2D_H2M
{
public:
	enum Type
	{
		Normal = 0,
		RenderTarget,
		DepthStencil,
	};

	DX11Texture2D();
	DX11Texture2D(const wchar_t* full_path);
	DX11Texture2D(const glm::vec2& size, DX11Texture2D::Type type);
	DX11Texture2D(const std::string& path, bool srgb = false, H2M::TextureWrapH2M wrap = H2M::TextureWrapH2M::Clamp);
	DX11Texture2D(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, const void* data, H2M::TextureWrapH2M wrap = H2M::TextureWrapH2M::Clamp);
	DX11Texture2D(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, H2M::TextureWrapH2M wrap = H2M::TextureWrapH2M::Clamp);
	virtual ~DX11Texture2D();

	void Invalidate();
	void Release();

	virtual uint32_t GetWidth() const override { return m_Width; }
	virtual uint32_t GetHeight() const override { return m_Height; }
	virtual void Bind(uint32_t slot = 0) const override;
	virtual H2M::RefH2M<H2M::Image2D_H2M> GetImage() const override;
	void Lock() override;
	void Unlock() override;

	virtual void Resize(uint32_t width, uint32_t height) override;

	H2M::BufferH2M GetWriteableBuffer() override;
	bool Loaded() const override;
	const std::string& GetPath() const override;
	H2M::ImageFormatH2M GetFormat() const override;
	uint32_t GetMipLevelCount() const override;
	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;
	virtual uint64_t GetHash() const; // { return (uint64_t)0; }

	// virtual H2M::RendererID_H2M GetRendererID() const override { return 0; } // Removed in Hazel Live 18.03.2021 #2

	bool operator ==(const HazelTexture& other) const override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual uint32_t GetID() const override { return 0; }

	virtual ImTextureID GetImTextureID() override { return (ImTextureID)(intptr_t)m_ShaderResourceViewDX11; };

	glm::vec2 getSize() { return m_Size; }
	DX11Texture2D::Type GetType() { return m_Type; }

	ID3D11RenderTargetView* GetRenderTargetViewDX11() { return m_RenderTargetViewDX11; }
	ID3D11DepthStencilView* GetDepthStencilViewDX11() { return m_DepthStencilViewDX11; }

private:
	std::string m_Path;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Channels;

	H2M::BufferH2M m_ImageData;

	H2M::ImageFormatH2M m_Format = H2M::ImageFormatH2M::None;

	DX11Texture2D::Type m_Type = DX11Texture2D::Type::Normal;
	glm::vec2 m_Size;

	ID3D11Resource* m_TextureDX11 = nullptr;
	ID3D11ShaderResourceView* m_ShaderResourceViewDX11 = nullptr;
	ID3D11RenderTargetView* m_RenderTargetViewDX11 = nullptr;
	ID3D11DepthStencilView* m_DepthStencilViewDX11 = nullptr;
	ID3D11SamplerState* m_SamplerStateDX11 = nullptr;

	friend class DX11Context;
	friend class DX11Renderer;
	friend class DX11VertexShader;
	friend class DX11PixelShader;

};
