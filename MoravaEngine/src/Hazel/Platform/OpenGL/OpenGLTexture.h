#pragma once

#include "../../Renderer/HazelTexture.h"


namespace Hazel {

	class OpenGLTexture2D : public HazelTexture2D
	{
	public:
		OpenGLTexture2D(HazelTextureFormat format, uint32_t width, uint32_t height, HazelTextureWrap wrap);
		OpenGLTexture2D(const std::string& path, bool srgb, HazelTextureWrap wrap);
		virtual ~OpenGLTexture2D();

		virtual void Bind(uint32_t slot = 0) const;

		virtual HazelTextureFormat GetFormat() const override { return m_Format; }
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		// This function currently returns the expected number of mips based on image size,
		// not present mips in data
		virtual uint32_t GetMipLevelCount() const override;

		virtual void Lock() override;
		virtual void Unlock() override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual Buffer GetWriteableBuffer() override;

		virtual const std::string& GetPath() const override { return m_FilePath; }

		virtual bool Loaded() const override { return m_Loaded; }

		virtual uint32_t GetID() const override { return m_ID; }

		virtual bool operator==(const HazelTexture& other) const override
		{
			return m_ID == ((OpenGLTexture2D&)other).m_ID;
		}
	private:
		uint32_t m_ID;
		HazelTextureFormat m_Format;
		HazelTextureWrap m_Wrap = HazelTextureWrap::Clamp;
		uint32_t m_Width, m_Height;

		Buffer m_ImageData;
		bool m_IsHDR = false;

		bool m_Locked = false;
		bool m_Loaded = false;

		std::string m_FilePath;

		float m_MaxAnisotropy = 16.0f;

	};

	class OpenGLTextureCube : public HazelTextureCube
	{
	public:
		OpenGLTextureCube(HazelTextureFormat format, uint32_t width, uint32_t height);
		OpenGLTextureCube(const std::string& path);
		virtual ~OpenGLTextureCube();

		virtual void Bind(uint32_t slot = 0) const;

		virtual HazelTextureFormat GetFormat() const { return m_Format; }
		virtual uint32_t GetWidth() const { return m_Width; }
		virtual uint32_t GetHeight() const { return m_Height; }
		// This function currently returns the expected number of mips based on image size,
		// not present mips in data
		virtual uint32_t GetMipLevelCount() const override;

		virtual const std::string& GetPath() const override { return m_FilePath; }

		virtual uint32_t GetID() const override { return m_ID; }

		virtual bool operator==(const HazelTexture& other) const override
		{
			return m_ID == ((OpenGLTextureCube&)other).m_ID;
		}
	private:
		uint32_t m_ID;
		HazelTextureFormat m_Format;
		uint32_t m_Width, m_Height;

		unsigned char* m_ImageData;

		std::string m_FilePath;

		float m_MaxAnisotropy = 16.0f;

	};
}
