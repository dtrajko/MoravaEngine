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
	DX11Texture2D(const std::string& path, bool srgb = false, Hazel::HazelTextureWrap wrap = Hazel::HazelTextureWrap::Clamp);
	DX11Texture2D(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data, Hazel::HazelTextureWrap wrap = Hazel::HazelTextureWrap::Clamp);
	DX11Texture2D(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, Hazel::HazelTextureWrap wrap = Hazel::HazelTextureWrap::Clamp);
	virtual ~DX11Texture2D();

	void Invalidate();

	virtual uint32_t GetWidth() const override { return m_Width; }
	virtual uint32_t GetHeight() const override { return m_Height; }

	virtual void Bind(uint32_t slot = 0) const override;

	virtual Hazel::Ref<Hazel::HazelImage2D> GetImage() const override { return m_Image; }

	void Lock() override;

	void Unlock() override;

	void Resize(uint32_t width, uint32_t height) override;

	Hazel::Buffer GetWriteableBuffer() override;

	bool Loaded() const override;

	const std::string& GetPath() const override;

	Hazel::HazelImageFormat GetFormat() const override;

	uint32_t GetMipLevelCount() const override;

	virtual uint64_t GetHash() const { return (uint64_t)m_Image; }
	virtual Hazel::RendererID GetRendererID() const override;

	bool operator ==(const HazelTexture& other) const override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual uint32_t GetID() const override { return uint32_t(0); /* Not implemented */ }

	glm::vec2 getSize() { return m_Size; }
	DX11Texture2D::Type GetType() { return m_Type; }

	ID3D11RenderTargetView* GetRenderTargetView() { return m_RenderTargetView; }
	ID3D11DepthStencilView* GetDepthStencilView() { return m_DepthStencilView; }

private:
	std::string m_Path;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Channels;

	Hazel::Buffer m_ImageData;

	VkDeviceMemory m_DeviceMemory;
	VkImage m_Image;

	Hazel::HazelImageFormat m_Format = Hazel::HazelImageFormat::None;


	glm::vec2 m_Size;
	DX11Texture2D::Type m_Type = DX11Texture2D::Type::Normal;

	ID3D11Resource* m_Texture = nullptr;
	ID3D11ShaderResourceView* m_ShaderResourceView = nullptr;
	ID3D11RenderTargetView* m_RenderTargetView = nullptr;
	ID3D11DepthStencilView* m_DepthStencilView = nullptr;
	ID3D11SamplerState* m_SamplerState = nullptr;

	friend class DX11Context;
	friend class DX11Renderer;

};
