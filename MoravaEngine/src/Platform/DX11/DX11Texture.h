#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelTexture.h"

#include "DX11.h"

#include "DX11Image.h"


class DX11Texture2D : public Hazel::HazelTexture2D
{
public:
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

private:
	std::string m_Path;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Channels;

	Hazel::Buffer m_ImageData;

	VkDeviceMemory m_DeviceMemory;
	VkImage m_Image;

	Hazel::HazelImageFormat m_Format = Hazel::HazelImageFormat::None;

};

class DX11TextureCube : public Hazel::HazelTextureCube
{
public:
	DX11TextureCube(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
	DX11TextureCube(const std::string& path);
	virtual ~DX11TextureCube();

	virtual const std::string& GetPath() const override { return ""; }

	virtual void Bind(uint32_t slot = 0) const override {}

	virtual Hazel::HazelImageFormat GetFormat() const { return m_Format; }

	virtual uint32_t GetWidth() const override { return m_Width; }
	virtual uint32_t GetHeight() const override { return m_Height; }
	virtual uint32_t GetMipLevelCount() const override;

	virtual bool operator==(const HazelTexture& other) const override { return false; }

	virtual uint64_t GetHash() const { return (uint64_t)m_Image; }

	void GenerateMips(bool readonly = false);

	// abstract methods in HazelTexture
	virtual uint32_t GetID() const override { return uint32_t(); /* Not implemented */ }
	virtual Hazel::RendererID GetRendererID() const override { return uint32_t(); /* Not implemented */ }
	// virtual void Resize(uint32_t width, uint32_t height) override { /* Not implemented */ };

private:
	void Invalidate();

private:
	Hazel::HazelImageFormat m_Format = Hazel::HazelImageFormat::None;
	uint32_t m_Width = 0, m_Height = 0;

	bool m_MipsGenerated = false;

	Hazel::Buffer m_LocalStorage;
	VkDeviceMemory m_DeviceMemory;
	VkImage m_Image;
	VkDescriptorImageInfo m_DescriptorImageInfo = {};

};
