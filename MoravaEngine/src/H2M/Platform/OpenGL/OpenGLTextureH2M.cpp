#include "OpenGLTexture.h"

#include "Hazel/Renderer/HazelRenderer.h"
#include "OpenGLImage.h"

#include "Core/Log.h"
#include "Core/Util.h"

#include <GL/glew.h>


namespace Hazel {

	/**** BEGIN obsolete method, remove later ****
	static GLenum HazelToOpenGLTextureFormat(ImageFormatH2M format)
	{
		switch (format)
		{
			case Hazel::ImageFormatH2M::RGB:     return GL_RGB;
			case Hazel::ImageFormatH2M::RGBA:    return GL_RGBA;
			case Hazel::ImageFormatH2M::RGBA16F: return GL_RGBA16F;
			case Hazel::ImageFormatH2M::RGBA32F: return GL_RGBA32F;
		}
		Log::GetLogger()->error("Unknown texture format!");
		return 0;
	}
	/**** END obsolete method, remove later ****/

	//////////////////////////////////////////////////////////////////////////////////
	// Texture2D
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLTexture2D::OpenGLTexture2D(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
		: m_Width(width), m_Height(height)
	{
		ImageSpecification imageSpec;
		imageSpec.Format = format;
		imageSpec.Width = width;
		imageSpec.Height = height;
		m_Image = HazelImage2D::Create(imageSpec, data);
		// RefH2M<OpenGLTexture2D> instance = this;
		// HazelRenderer::Submit([=]() {});
		{
			m_Image->Invalidate();
		}
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path, TextureProperties properties)
		: m_FilePath(path)
	{
		int width, height, channels;
		if (stbi_is_hdr(path.c_str()))
		{
			// HZ_CORE_INFO("Loading texture {0}, srgb={1}", path, srgb);
			Log::GetLogger()->info("Loading HDR texture {0}, srgb={1}", path, properties.SRGB);

			float* imageData = stbi_loadf(path.c_str(), &width, &height, &channels, 0); // STBI_rgb_alpha

			// H2M_CORE_ASSERT(imageData);
			if (!imageData) { Log::GetLogger()->error("Could not read image!"); }

			m_Format = ImageFormatH2M::RGBA16F;

			Buffer buffer(imageData, Utils::GetImageMemorySize(m_Format, width, height));
			ImageSpecification imageSpec;
			imageSpec.Format = m_Format;
			imageSpec.Width = width;
			imageSpec.Height = height;
			m_Image = HazelImage2D::Create(imageSpec, buffer);

			m_IsHDR = true;
		}
		else
		{
			// HZ_CORE_INFO("Loading texture {0}, srgb={1}", path, srgb);
			Log::GetLogger()->info("Loading texture {0}, srgb={1}", path, properties.SRGB);

			stbi_uc* imageData = stbi_load(path.c_str(), &width, &height, &channels, properties.SRGB ? STBI_rgb : STBI_rgb_alpha);

			// H2M_CORE_ASSERT(imageData);
			if (!imageData) { Log::GetLogger()->error("Could not read image!"); }

			// ImageFormatH2M format = channels == 4 ? ImageFormatH2M::RGBA : ImageFormatH2M::RGB;
			m_Format = properties.SRGB ? ImageFormatH2M::RGB : ImageFormatH2M::RGBA;

			Buffer buffer(imageData, Utils::GetImageMemorySize(m_Format, width, height));
			ImageSpecification imageSpec;
			imageSpec.Format = m_Format;
			imageSpec.Width = width;
			imageSpec.Height = height;
			m_Image = HazelImage2D::Create(imageSpec, buffer);

			m_IsHDR = false;
		}

		m_Width = width;
		m_Height = height;
		m_Loaded = true;

		RefH2M<HazelImage2D>& image = m_Image;
		// HazelRenderer::Submit([image]() mutable {});
		{
			// image->Invalidate();
			image.As<OpenGLImage2D>()->InvalidateOld(properties.SRGB, m_Wrap, m_IsHDR);

			Buffer& buffer = image->GetBuffer();
			stbi_image_free(buffer.Data);
			buffer = Buffer();
		}

		Util::CheckOpenGLErrors("OpenGLTexture2D::OpenGLTexture2D");
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		// RefH2M<HazelImage2D> image = m_Image;
		// HazelRenderer::Submit([image]() mutable {});
		{
			m_Image->Release();
		}
		// glDeleteTextures(1, &m_ID);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		RefH2M<OpenGLImage2D> image = m_Image.As<OpenGLImage2D>();
		// HazelRenderer::Submit([slot, image ]() {});
		{
			glBindTextureUnit(slot, image->GetRendererID());
		}
	}

	void OpenGLTexture2D::Lock()
	{
		m_Locked = true;
	}

	void OpenGLTexture2D::Unlock()
	{
		m_Locked = false;

		// RefH2M<OpenGLTexture2D> instance = this;
		// RefH2M<OpenGLImage2D> image = m_Image.As<OpenGLImage2D>();
		// HazelRenderer::Submit([instance, image]() mutable {});
		{
			GLenum format = Utils::OpenGLImageFormat(m_Format);
			glTextureSubImage2D(m_Image.As<OpenGLImage2D>()->GetRendererID(), 0, 0, 0, m_Width, m_Height, format, GL_UNSIGNED_BYTE, m_Image->GetBuffer().Data);
		}
	}

	Buffer OpenGLTexture2D::GetWriteableBuffer()
	{
		// H2M_CORE_ASSERT(m_Locked, "Texture must be locked!");
		if (!m_Locked) { Log::GetLogger()->error("Texture must be locked!"); }
		// return m_ImageData;
		return m_Image->GetBuffer();
	}

	void OpenGLTexture2D::Resize(uint32_t width, uint32_t height)
	{
		Log::GetLogger()->error("OpenGLTexture2D::Resize({0},{1}) method not yet implemented!", width, height);
	}

	uint32_t OpenGLTexture2D::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(m_Width, m_Height);
	}

	std::pair<uint32_t, uint32_t> OpenGLTexture2D::GetMipSize(uint32_t mip) const
	{
		Log::GetLogger()->error("OpenGLTexture2D::GetMipSize({0}) - method not implemented!", mip);
		return std::pair<uint32_t, uint32_t>();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// TextureCube
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLTextureCube::OpenGLTextureCube(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
	{
		m_Width = width;
		m_Height = height;
		m_Format = format;

		if (data)
		{
			uint32_t size = width * height * 4 * 6; // six layers
			m_LocalStorage = Buffer::Copy(data, size);
		}

		uint32_t levels = Utils::CalculateMipCount(width, height);
		// RefH2M<OpenGLTextureCube> instance = this;
		// HazelRenderer::Submit([instance, levels]() mutable {});
		{
			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, levels, Utils::OpenGLImageInternalFormat(m_Format), m_Width, m_Height);
			if (m_LocalStorage.Data)
			{
				glTextureSubImage3D(m_RendererID, 0, 0, 0, 0, m_Width, m_Height, 6, Utils::OpenGLImageFormat(m_Format), Utils::OpenGLFormatDataType(m_Format), m_LocalStorage.Data);
			}

			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
		}
	}

	OpenGLTextureCube::OpenGLTextureCube(ImageFormatH2M format, uint32_t width, uint32_t height, bool notUsed)
	{
		m_Width = width;
		m_Height = height;
		m_Format = format;

		uint32_t levels = Utils::CalculateMipCount(width, height);

		GLenum imageFormat = Utils::OpenGLImageInternalFormat(m_Format);

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, levels, imageFormat, width, height);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
		m_Format = ImageFormatH2M::RGB;

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

		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glTextureParameterf(m_RendererID, GL_TEXTURE_MAX_ANISOTROPY, m_MaxAnisotropy);

		auto internalFormat = Utils::OpenGLImageInternalFormat(m_Format);
		auto format = Utils::OpenGLImageFormat(m_Format);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalFormat, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[2]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalFormat, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[0]);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalFormat, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[4]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalFormat, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[5]);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalFormat, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[1]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalFormat, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[3]);

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		glBindTexture(GL_TEXTURE_2D, 0);

		for (size_t i = 0; i < faces.size(); i++)
		{
			delete[] faces[i];
		}

		stbi_image_free(m_ImageData);
	}

	OpenGLTextureCube::~OpenGLTextureCube()
	{
		auto self = this;
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTextureCube::Bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_RendererID);
	}

	uint32_t OpenGLTextureCube::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(m_Width, m_Height);
	}

	std::pair<uint32_t, uint32_t> OpenGLTextureCube::GetMipSize(uint32_t mip) const
	{
		Log::GetLogger()->error("OpenGLTextureCube::GetMipSize({0}) - method not implemented!", mip);
		return std::pair<uint32_t, uint32_t>();
	}

}
