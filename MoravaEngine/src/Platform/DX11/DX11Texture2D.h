#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelTexture.h"

#include "DX11.h"

#include "DX11Image.h"


class DX11Texture2D : public Hazel::HazelTexture2D
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
	DX11Texture2D(const std::string& path, bool srgb = false, Hazel::TextureWrap wrap = Hazel::TextureWrap::Clamp);
	DX11Texture2D(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data, Hazel::TextureWrap wrap = Hazel::TextureWrap::Clamp);
	DX11Texture2D(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, Hazel::TextureWrap wrap = Hazel::TextureWrap::Clamp);
	virtual ~DX11Texture2D();

	void Invalidate();
	void Release();

	virtual uint32_t GetWidth() const override { return m_Width; }
	virtual uint32_t GetHeight() const override { return m_Height; }
	virtual void Bind(uint32_t slot = 0) const override;
	virtual Hazel::Ref<Hazel::HazelImage2D> GetImage() const override;
	void Lock() override;
	void Unlock() override;
	void Resize(uint32_t width, uint32_t height) override;
	Hazel::Buffer GetWriteableBuffer() override;
	bool Loaded() const override;
	const std::string& GetPath() const override;
	Hazel::HazelImageFormat GetFormat() const override;
	uint32_t GetMipLevelCount() const override;
	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;
	virtual uint64_t GetHash() const; // { return (uint64_t)0; }
	virtual Hazel::RendererID GetRendererID() const override;

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

	Hazel::Buffer m_ImageData;

	Hazel::HazelImageFormat m_Format = Hazel::HazelImageFormat::None;

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
