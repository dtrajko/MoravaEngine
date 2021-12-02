#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Platform/OpenGL/OpenGLImage.h"
#include "Hazel/Renderer/RendererTypes.h"
#include "Hazel/Renderer/HazelTexture.h"


namespace Hazel {

	class OpenGLTexture2D : public HazelTexture2D
	{
	public:
		OpenGLTexture2D(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data);
		OpenGLTexture2D(const std::string& path, TextureProperties properties);
		// OpenGLTexture2D(ImageFormatH2M format, uint32_t width, uint32_t height, TextureWrap wrap);
		virtual ~OpenGLTexture2D();

		virtual void Bind(uint32_t slot = 0) const;

		virtual ImageFormatH2M GetFormat() const { return m_Format; }
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		// This function currently returns the expected number of mips based on image size,
		// not present mips in data
		virtual uint32_t GetMipLevelCount() const override;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

		virtual RefH2M<HazelImage2D> GetImage() const override { return m_Image; }

		virtual void Lock() override;
		virtual void Unlock() override;

		virtual Buffer GetWriteableBuffer() override;

		void Resize(uint32_t width, uint32_t height);

		virtual const std::string& GetPath() const override { return m_FilePath; }

		virtual bool Loaded() const override { return m_Loaded; }

		virtual uint64_t GetHash() const { return m_Image->GetHash(); }

		virtual bool operator==(const HazelTexture& other) const override
		{
			// return m_Image->GetHash() == other.m_Image->GetHash();
			return m_Image.As<OpenGLImage2D>()->GetRendererID() == ((OpenGLTexture2D&)other).GetImage().As<OpenGLImage2D>()->GetRendererID();
		}

		virtual uint32_t GetID() const override { return m_Image.As<OpenGLImage2D>()->GetRendererID(); }

		// virtual RendererID GetRendererID() const override { return m_RendererID; } // Removed in Hazel Live 18.03.2021 #2


	private:
		RefH2M<HazelImage2D> m_Image;
		ImageFormatH2M m_Format;
		TextureWrap m_Wrap = TextureWrap::Clamp;
		uint32_t m_Width, m_Height;

		// RendererID m_RendererID; // Removed in Hazel Live 18.03.2021 #2
		Buffer m_ImageData; // TODO: remove in Hazel Live 18.03.2021 #2

		bool m_IsHDR = false;

		bool m_Locked = false;
		bool m_Loaded = false;

		std::string m_FilePath;

		float m_MaxAnisotropy = 16.0f;

	};

	class OpenGLTextureCube : public HazelTextureCube
	{
	public:
		OpenGLTextureCube(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data = nullptr);
		OpenGLTextureCube(ImageFormatH2M format, uint32_t width, uint32_t height, bool notUsed);
		OpenGLTextureCube(const std::string& path);
		virtual ~OpenGLTextureCube();

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

		virtual bool operator==(const HazelTexture& other) const override
		{
			return m_RendererID == ((OpenGLTextureCube&)other).m_RendererID;
		}

		RendererID GetRendererID() const { return m_RendererID; }

	private:
		RendererID m_RendererID;
		ImageFormatH2M m_Format;
		uint32_t m_Width, m_Height;

		Buffer m_LocalStorage;

		std::string m_FilePath;

		unsigned char* m_ImageData;

		float m_MaxAnisotropy = 16.0f;

	};

}
