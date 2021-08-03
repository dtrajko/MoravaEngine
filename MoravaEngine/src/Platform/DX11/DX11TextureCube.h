#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelTexture.h"

#include "DX11.h"

#include "DX11Image.h"


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
	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override { return { 0, 0 }; }

	virtual bool operator==(const HazelTexture& other) const override { return false; }

	virtual uint64_t GetHash() const { return (uint64_t)0; /* Not implemented */ }

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

};
