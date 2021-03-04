#include "OpenGLTexture.h"

#include "Core/Log.h"

#include <GL/glew.h>


namespace Hazel {

	static GLenum HazelToOpenGLTextureFormat(HazelTextureFormat format)
	{
		switch (format)
		{
			case Hazel::HazelTextureFormat::RGB:     return GL_RGB;
			case Hazel::HazelTextureFormat::RGBA:    return GL_RGBA;
			case Hazel::HazelTextureFormat::Float16: return GL_RGBA16F;
		}
		Log::GetLogger()->error("Unknown texture format!");
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Texture2D
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLTexture2D::OpenGLTexture2D(HazelTextureFormat format, uint32_t width, uint32_t height, HazelTextureWrap wrap)
		: m_Format(format), m_Width(width), m_Height(height), m_Wrap(wrap)
	{
		auto self = this;

		glGenTextures(1, &m_ID);
		glBindTexture(GL_TEXTURE_2D, m_ID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLenum localWrap = (m_Wrap == HazelTextureWrap::Clamp) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)localWrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)localWrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, (GLint)localWrap);

		glTextureParameterf(m_ID, GL_TEXTURE_MAX_ANISOTROPY, m_MaxAnisotropy);

		glTexImage2D(GL_TEXTURE_2D, 0, HazelToOpenGLTextureFormat(m_Format), m_Width, m_Height, 0, HazelToOpenGLTextureFormat(m_Format), GL_UNSIGNED_BYTE, nullptr);

		glBindTexture(GL_TEXTURE_2D, 0);

		m_ImageData.Allocate(width * height * HazelTexture::GetBPP(m_Format));
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path, bool srgb, HazelTextureWrap wrap)
		: m_FilePath(path), m_Wrap(wrap)
	{
		int width, height, channels;
		if (stbi_is_hdr(path.c_str()))
		{
			Log::GetLogger()->info("Loading HDR texture {0}, srgb={1}", path, srgb);
			m_ImageData.Data = (byte*)stbi_loadf(path.c_str(), &width, &height, &channels, 0);
			m_IsHDR = true;
			m_Format = HazelTextureFormat::Float16;
		}
		else
		{
			Log::GetLogger()->info("Loading texture {0}, srgb={1}", path, srgb);
			m_ImageData.Data = stbi_load(path.c_str(), &width, &height, &channels, srgb ? STBI_rgb : STBI_rgb_alpha);
			if (!m_ImageData.Data) {
				Log::GetLogger()->error("Could not read image!");
			}
			m_Format = HazelTextureFormat::RGBA;
		}

		if (!m_ImageData.Data)
			return;

		m_Loaded = true;

		m_Width = width;
		m_Height = height;

		// TODO: Consolidate properly
		if (srgb)
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &m_ID);
			int levels = HazelTexture::CalculateMipMapCount(m_Width, m_Height);
			glTextureStorage2D(m_ID, levels, GL_SRGB8, m_Width, m_Height);
			glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureSubImage2D(m_ID, 0, 0, 0, m_Width, m_Height, GL_RGB, GL_UNSIGNED_BYTE, m_ImageData.Data);
			glGenerateTextureMipmap(m_ID);
		}
		else
		{
			glGenTextures(1, &m_ID);
			glBindTexture(GL_TEXTURE_2D, m_ID);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			GLenum localWrap = (m_Wrap == HazelTextureWrap::Clamp) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)localWrap);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)localWrap);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, (GLint)localWrap);

			GLenum internalFormat = HazelToOpenGLTextureFormat(m_Format);
			GLenum format = srgb ? GL_SRGB8 : (m_IsHDR ? GL_RGB : HazelToOpenGLTextureFormat(m_Format)); // HDR = GL_RGB for now
			GLenum type = internalFormat == GL_RGBA16F ? GL_FLOAT : GL_UNSIGNED_BYTE;
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, type, m_ImageData.Data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glBindTexture(GL_TEXTURE_2D, 0);
		}
		stbi_image_free(m_ImageData.Data);
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		glDeleteTextures(1, &m_ID);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_ID);
	}

	void OpenGLTexture2D::Lock()
	{
		m_Locked = true;
	}

	void OpenGLTexture2D::Unlock()
	{
		m_Locked = false;
		glTextureSubImage2D(m_ID, 0, 0, 0, m_Width, m_Height, HazelToOpenGLTextureFormat(m_Format), GL_UNSIGNED_BYTE, m_ImageData.Data);
	}

	void OpenGLTexture2D::Resize(uint32_t width, uint32_t height)
	{
		if (!m_Locked) {
			Log::GetLogger()->error("Texture must be locked!");
		}

		m_ImageData.Allocate(width * height * HazelTexture::GetBPP(m_Format));
#if HZ_DEBUG
		m_ImageData.ZeroInitialize();
#endif
	}

	Buffer OpenGLTexture2D::GetWriteableBuffer()
	{
		if (!m_Locked) {
			Log::GetLogger()->error("Texture must be locked!");
		}
		return m_ImageData;
	}

	uint32_t OpenGLTexture2D::GetMipLevelCount() const
	{
		return HazelTexture::CalculateMipMapCount(m_Width, m_Height);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// TextureCube
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLTextureCube::OpenGLTextureCube(HazelTextureFormat format, uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
		m_Format = format;

		uint32_t levels = HazelTexture::CalculateMipMapCount(width, height);

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_ID);
		glTextureStorage2D(m_ID, levels, HazelToOpenGLTextureFormat(m_Format), width, height);
		glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		// glTextureParameterf(m_RendererID, GL_TEXTURE_MAX_ANISOTROPY, 16);
	}

	OpenGLTextureCube::OpenGLTextureCube(const std::string& path)
		: m_FilePath(path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(false);
		m_ImageData = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb);

		m_Width = width;
		m_Height = height;
		m_Format = HazelTextureFormat::RGB;

		uint32_t faceWidth = m_Width / 4;
		uint32_t faceHeight = m_Height / 3;

		if (faceWidth != faceHeight) {
			Log::GetLogger()->error("Non-square faces!");
		}

		std::array<unsigned char*, 6> faces;
		for (size_t i = 0; i < faces.size(); i++)
			faces[i] = new unsigned char[faceWidth * faceHeight * 3]; // 3 BPP

		int faceIndex = 0;

		for (size_t i = 0; i < 4; i++)
		{
			for (size_t y = 0; y < faceHeight; y++)
			{
				size_t yOffset = y + faceHeight;
				for (size_t x = 0; x < faceWidth; x++)
				{
					size_t xOffset = x + i * faceWidth;
					faces[faceIndex][(x + y * faceWidth) * 3 + 0] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 0];
					faces[faceIndex][(x + y * faceWidth) * 3 + 1] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 1];
					faces[faceIndex][(x + y * faceWidth) * 3 + 2] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 2];
				}
			}
			faceIndex++;
		}

		for (size_t i = 0; i < 3; i++)
		{
			// Skip the middle one
			if (i == 1)
				continue;

			for (size_t y = 0; y < faceHeight; y++)
			{
				size_t yOffset = y + i * faceHeight;
				for (size_t x = 0; x < faceWidth; x++)
				{
					size_t xOffset = x + faceWidth;
					faces[faceIndex][(x + y * faceWidth) * 3 + 0] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 0];
					faces[faceIndex][(x + y * faceWidth) * 3 + 1] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 1];
					faces[faceIndex][(x + y * faceWidth) * 3 + 2] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 2];
				}
			}
			faceIndex++;
		}

		glGenTextures(1, &m_ID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glTextureParameterf(m_ID, GL_TEXTURE_MAX_ANISOTROPY, m_MaxAnisotropy);

		auto format = HazelToOpenGLTextureFormat(m_Format);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[2]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[0]);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[4]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[5]);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[1]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[3]);

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		glBindTexture(GL_TEXTURE_2D, 0);

		for (size_t i = 0; i < faces.size(); i++)
			delete[] faces[i];

		stbi_image_free(m_ImageData);
	}

	OpenGLTextureCube::~OpenGLTextureCube()
	{
		auto self = this;
		glDeleteTextures(1, &m_ID);
	}

	void OpenGLTextureCube::Bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_ID);
	}

	uint32_t OpenGLTextureCube::GetMipLevelCount() const
	{
		return HazelTexture::CalculateMipMapCount(m_Width, m_Height);
	}

}
