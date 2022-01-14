/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/RendererTypesH2M.h"
#include "H2M/Renderer/TextureH2M.h"


namespace H2M
{

	class OpenGLTexture2D_H2M : public Texture2D_H2M
	{
	public:
		OpenGLTexture2D_H2M(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data);
		OpenGLTexture2D_H2M(const std::string& path, bool srgb);
		OpenGLTexture2D_H2M(const std::string& path, TexturePropertiesH2M properties = TexturePropertiesH2M());
		// OpenGLTexture2D(HazelImageFormat format, uint32_t width, uint32_t height, TextureWrap wrap);
		virtual ~OpenGLTexture2D_H2M();

		virtual void Bind(uint32_t slot = 0) const;

		virtual RefH2M<Image2D_H2M> GetImage() const override { return m_Image; }

		virtual ImageFormatH2M GetFormat() const override { return m_Image->GetFormat(); }
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		// This function currently returns the expected number of mips based on image size,
		// not present mips in data
		virtual uint32_t GetMipLevelCount() const override;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

		virtual void Lock() override;
		virtual void Unlock() override;

		virtual BufferH2M GetWriteableBuffer() override;

		virtual const std::string& GetPath() const override { return m_FilePath; }

		virtual bool Loaded() const override { return m_Loaded; }

		virtual uint64_t GetHash() const { return m_Image->GetHash(); }

		virtual bool operator==(const TextureH2M& other) const override
		{
			// return m_Image->GetHash() == other.m_Image->GetHash();
			return m_RendererID == ((OpenGLTexture2D_H2M&)other).m_RendererID;
		}

		virtual uint32_t GetID() const override { return m_RendererID; }

		// virtual RendererID GetRendererID() const override { return m_RendererID; } // Removed in Hazel Live 18.03.2021 #2

		// void Resize(uint32_t width, uint32_t height); // method removed in Hazel Live 18.03.2021 #2

	private:
		RefH2M<Image2D_H2M> m_Image;
		ImageFormatH2M m_Format;
		TextureWrapH2M m_Wrap = TextureWrapH2M::Clamp;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;

		BufferH2M m_ImageData;
		bool m_IsHDR = false;

		bool m_Locked = false;
		bool m_Loaded = false;

		std::string m_FilePath;

		float m_MaxAnisotropy = 16.0f;

	};

	class OpenGLTextureCubeH2M : public TextureCubeH2M
	{
	public:
		OpenGLTextureCubeH2M(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data = nullptr);
		OpenGLTextureCubeH2M(ImageFormatH2M format, uint32_t width, uint32_t height, bool notUsed);
		OpenGLTextureCubeH2M(const std::string& path);
		virtual ~OpenGLTextureCubeH2M();

		virtual void Bind(uint32_t slot = 0) const;

		virtual ImageFormatH2M GetFormat() const { return m_Format; }
		virtual uint32_t GetWidth() const { return m_Width; }
		virtual uint32_t GetHeight() const { return m_Height; }
		// This function currently returns the expected number of mips based on image size,
		// not present mips in data
		virtual uint32_t GetMipLevelCount() const override;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

		virtual const std::string& GetPath() const override { return m_FilePath; }

		virtual uint32_t GetID() const override { return m_RendererID; }
		virtual uint64_t GetHash() const { return (uint64_t)m_RendererID; }

		virtual bool operator==(const TextureH2M& other) const override
		{
			return m_RendererID == ((OpenGLTextureCubeH2M&)other).m_RendererID;
		}

		uint32_t GetRendererID() const { return m_RendererID; }

	private:
		uint32_t m_RendererID;
		ImageFormatH2M m_Format;
		uint32_t m_Width, m_Height;

		BufferH2M m_LocalStorage;

		std::string m_FilePath;

		unsigned char* m_ImageData;

		float m_MaxAnisotropy = 16.0f;

	};

}
