/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Renderer/TextureH2M.h"


namespace H2M
{

	class OpenGLTexture2D_Hazel2D : public Texture2D_H2M
	{
	public:
		OpenGLTexture2D_Hazel2D(uint32_t width, uint32_t height);
		OpenGLTexture2D_Hazel2D(const std::string& path);
		virtual ~OpenGLTexture2D_Hazel2D();

		virtual uint32_t GetWidth() const override { return m_Width;  }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const { return m_RendererID; }
		
		virtual void SetData(void* data, uint32_t size) override;

		virtual void Bind(uint32_t slot = 0) const override;

		virtual ImageFormatH2M GetFormat() const override { return ImageFormatH2M::None; }
		virtual uint32_t GetMipLevelCount() const override { return uint32_t(); }
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override { return std::pair<uint32_t, uint32_t>(); }
		virtual uint64_t GetHash() const { return uint64_t(); }
		virtual uint32_t GetID() const override { return m_RendererID; }
		virtual RefH2M<Image2D_H2M> GetImage() const override { return RefH2M<Image2D_H2M>(); }
		virtual void Lock() override {};
		virtual void Unlock() override {};
		virtual BufferH2M GetWriteableBuffer() override { return BufferH2M(); }
		virtual const std::string& GetPath() const override { return m_Path; }

		virtual bool Loaded() const override { return m_IsLoaded; }

		virtual bool operator==(const TextureH2M& other) const override
		{
			return m_RendererID == ((OpenGLTexture2D_Hazel2D&)other).m_RendererID;
		}

	private:
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
	};

}
