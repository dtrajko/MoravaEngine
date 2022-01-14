/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLTextureH2M.h"

#include "H2M/Renderer/RendererH2M.h"
#include "OpenGLImageH2M.h"

#include "Core/Log.h"
#include "Core/Util.h"

#include <GL/glew.h>


namespace H2M
{

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

	OpenGLTexture2D_H2M::OpenGLTexture2D_H2M(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
		: m_Width(width), m_Height(height)
	{
		m_Image = Image2D_H2M::Create(format, width, height, data);
		// RefH2M<OpenGLTexture2D_H2M> instance = this;
		// HazelRenderer::Submit([=]() {});
		{
			m_Image->Invalidate();
		}
	}

	OpenGLTexture2D_H2M::OpenGLTexture2D_H2M(const std::string& path, bool srgb)
		: m_FilePath(path)
	{
#if 0
		int width, height, channels;
		if (stbi_is_hdr(path.c_str()))
		{
			// HZ_CORE_INFO("Loading texture {0}, srgb={1}", path, srgb);
			Log::GetLogger()->info("Loading HDR texture {0}, srgb={1}", path, srgb);

			float* imageData = stbi_loadf(path.c_str(), &width, &height, &channels, 0);

			// HZ_CORE_ASSERT(imageData);
			if (!imageData) { Log::GetLogger()->error("Could not read image!"); }

			Buffer buffer(imageData, Utils::GetImageMemorySize(ImageFormatH2M::RGBA32F, width, height));
			m_Image = HazelImage2D::Create(ImageFormatH2M::RGBA32F, width, height, buffer);

			// m_ImageData.Data = imageData;
			m_IsHDR = true;
			m_Format = ImageFormatH2M::RGBA16F;

		}
		else
		{
			// HZ_CORE_INFO("Loading texture {0}, srgb={1}", path, srgb);
			Log::GetLogger()->info("Loading texture {0}, srgb={1}", path, srgb);

			stbi_uc* imageData = stbi_load(path.c_str(), &width, &height, &channels, srgb ? STBI_rgb : STBI_rgb_alpha);

			// HZ_CORE_ASSERT(imageData);
			if (!imageData) { Log::GetLogger()->error("Could not read image!"); }

			// ImageFormatH2M format = srgb ? ImageFormatH2M::RGB : ImageFormatH2M::RGBA;
			ImageFormatH2M format = channels == 4 ? ImageFormatH2M::RGBA : ImageFormatH2M::RGB;

			Buffer buffer(imageData, Utils::GetImageMemorySize(format, width, height));
			m_Image = HazelImage2D::Create(format, width, height, buffer);
		}

		if (!m_ImageData.Data)
		{
			// return;
		}

		m_Width = width;
		m_Height = height;
		m_Loaded = true;

		// RefH2M<HazelImage2D>& image = m_Image;
		// HazelRenderer::Submit([image]() mutable {});
		{
			m_Image->Invalidate();

			Buffer& buffer = m_Image->GetBuffer();
			stbi_image_free(buffer.Data);
			buffer = Buffer();
		}
#else
		int width, height, channels;
		if (stbi_is_hdr(path.c_str()))
		{
			// HZ_CORE_INFO("Loading texture {0}, srgb={1}", path, srgb);
			Log::GetLogger()->info("Loading HDR texture {0}, srgb={1}", path, srgb);
			m_ImageData.Data = (byte*)stbi_loadf(path.c_str(), &width, &height, &channels, 0);
			m_IsHDR = true;
			m_Format = ImageFormatH2M::RGBA16F;

			BufferH2M buffer(m_ImageData.Data, Utils::GetImageMemorySize(ImageFormatH2M::RGBA32F, width, height));
			m_Image = Image2D_H2M::Create(ImageFormatH2M::RGBA32F, width, height, buffer);
		}
		else
		{
			// HZ_CORE_INFO("Loading texture {0}, srgb={1}", path, srgb);
			Log::GetLogger()->info("Loading texture {0}, srgb={1}", path, srgb);
			m_ImageData.Data = stbi_load(path.c_str(), &width, &height, &channels, srgb ? STBI_rgb : STBI_rgb_alpha);
			if (!m_ImageData.Data)
			{
				Log::GetLogger()->error("Could not read image!");
			}
			m_Format = ImageFormatH2M::RGBA;

			ImageFormatH2M format = srgb ? ImageFormatH2M::RGB : ImageFormatH2M::RGBA;
			BufferH2M buffer(m_ImageData.Data, Utils::GetImageMemorySize(format, width, height));
			m_Image = Image2D_H2M::Create(format, width, height, buffer);
		}

		if (!m_ImageData.Data)
		{
			return;
		}

		m_Width = width;
		m_Height = height;
		m_Loaded = true;

		// RefH2M<HazelImage2D>& image = m_Image;
		// HazelRenderer::Submit([image]() mutable {});
		{
			// m_Image->Invalidate();

			BufferH2M& buffer = m_Image->GetBuffer();
			// stbi_image_free(buffer.Data);
			buffer = BufferH2M();
		}

		/**** BEGIN this part of the code should be removed from the constructor in Vulkan branch ****/

		// TODO: Consolidate properly
		if (srgb)
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			int levels = Utils::CalculateMipCount(m_Width, m_Height);
			glTextureStorage2D(m_RendererID, levels, GL_SRGB8, m_Width, m_Height);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, GL_RGB, GL_UNSIGNED_BYTE, m_ImageData.Data);
			glGenerateTextureMipmap(m_RendererID);
	}
		else
		{
			glGenTextures(1, &m_RendererID);
			glBindTexture(GL_TEXTURE_2D, m_RendererID);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			GLenum localWrap = (m_Wrap == TextureWrapH2M::Clamp) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)localWrap);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)localWrap);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, (GLint)localWrap);

			// GLenum internalFormat = HazelToOpenGLTextureFormat(m_Format);
			// GLenum format = srgb ? GL_SRGB8 : (m_IsHDR ? GL_RGB : HazelToOpenGLTextureFormat(m_Format)); // HDR = GL_RGB for now
			GLenum internalFormat = Utils::OpenGLImageInternalFormat(m_Format);
			GLenum format = srgb ? GL_SRGB8 : (m_IsHDR ? GL_RGB : Utils::OpenGLImageFormat(m_Format)); // HDR = GL_RGB for no
			GLenum type = internalFormat == GL_RGBA16F ? GL_FLOAT : GL_UNSIGNED_BYTE;
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, type, m_ImageData.Data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		stbi_image_free(m_ImageData.Data);
		/**** END this part of the code should be removed from the constructor in Vulkan branch ****/
#endif
		Util::CheckOpenGLErrors("OpenGLTexture2D_H2M::OpenGLTexture2D_H2M");
	}

	OpenGLTexture2D_H2M::OpenGLTexture2D_H2M(const std::string& path, TexturePropertiesH2M properties)
	{
	}

	/**** BEGIN Method removed in Vulkan branch ****
	OpenGLTexture2D_H2M::OpenGLTexture2D_H2M(ImageFormatH2M format, uint32_t width, uint32_t height, TextureWrap wrap)
		: m_Format(format), m_Width(width), m_Height(height), m_Wrap(wrap)
	{
		auto self = this;

		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLenum localWrap = (m_Wrap == TextureWrap::Clamp) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)localWrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)localWrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, (GLint)localWrap);

		glTextureParameterf(m_RendererID, GL_TEXTURE_MAX_ANISOTROPY, m_MaxAnisotropy);

		GLenum imageInternalFormat = Utils::OpenGLImageInternalFormat(m_Format);
		GLenum imageFormat = Utils::OpenGLImageFormat(m_Format);
		glTexImage2D(GL_TEXTURE_2D, 0, imageInternalFormat, m_Width, m_Height, 0, imageFormat, GL_UNSIGNED_BYTE, nullptr);

		glBindTexture(GL_TEXTURE_2D, 0);

		m_ImageData.Allocate(width * height * HazelTexture::GetBPP(m_Format));
	}
	/**** END Method removed in Vulkan branch ****/

	OpenGLTexture2D_H2M::~OpenGLTexture2D_H2M()
	{
		// RefH2M<HazelImage2D> image = m_Image;
		// HazelRenderer::Submit([image]() mutable {});
		{
			m_Image->Release();
		}
		// glDeleteTextures(1, &m_ID);
	}

	void OpenGLTexture2D_H2M::Bind(uint32_t slot) const
	{
		// RefH2M<OpenGLImage2D> image = m_Image.As<OpenGLImage2D>();
		// HazelRenderer::Submit([slot, image ]() {});
		{
			glBindTextureUnit(slot, m_RendererID);
		}
	}

	void OpenGLTexture2D_H2M::Lock()
	{
		m_Locked = true;
	}

	void OpenGLTexture2D_H2M::Unlock()
	{
		m_Locked = false;

		// RefH2M<OpenGLTexture2D_H2M> instance = this;
		// RefH2M<OpenGLImage2D> image = m_Image.As<OpenGLImage2D>();
		// HazelRenderer::Submit([instance, image]() mutable {});
		{
			GLenum format = Utils::OpenGLImageFormat(m_Format);
			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, format, GL_UNSIGNED_BYTE, m_Image->GetBuffer().Data);
		}
	}

	/**** BEGIN method removed in Hazel Live 18.03.2021 #2 ****
	void OpenGLTexture2D_H2M::Resize(uint32_t width, uint32_t height)
	{
		// HZ_CORE_ASSERT(m_Locked, "Texture must be locked!");
		if (!m_Locked) { Log::GetLogger()->error("Texture must be locked!"); }
		m_ImageData.Allocate(width * height * HazelTexture::GetBPP(m_Format));
#if HZ_DEBUG
		m_ImageData.ZeroInitialize();
#endif
	}
	/**** END method removed in Hazel Live 18.03.2021 #2 ****/

	BufferH2M OpenGLTexture2D_H2M::GetWriteableBuffer()
	{
		// HZ_CORE_ASSERT(m_Locked, "Texture must be locked!");
		if (!m_Locked) { Log::GetLogger()->error("Texture must be locked!"); }
		// return m_ImageData;
		return m_Image->GetBuffer();
	}

	uint32_t OpenGLTexture2D_H2M::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(m_Width, m_Height);
	}

	std::pair<uint32_t, uint32_t> OpenGLTexture2D_H2M::GetMipSize(uint32_t mip) const
	{
		Log::GetLogger()->error("OpenGLTexture2D_H2M::GetMipSize({0}) - method not implemented!", mip);
		return std::pair<uint32_t, uint32_t>();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// TextureCube
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLTextureCubeH2M::OpenGLTextureCubeH2M(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
	{
		m_Width = width;
		m_Height = height;
		m_Format = format;

		if (data)
		{
			uint32_t size = width * height * 4 * 6; // six layers
			m_LocalStorage = BufferH2M::Copy(data, size);
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

	OpenGLTextureCubeH2M::OpenGLTextureCubeH2M(ImageFormatH2M format, uint32_t width, uint32_t height, bool notUsed)
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

	OpenGLTextureCubeH2M::OpenGLTextureCubeH2M(const std::string& path)
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

	OpenGLTextureCubeH2M::~OpenGLTextureCubeH2M()
	{
		auto self = this;
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTextureCubeH2M::Bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_RendererID);
	}

	uint32_t OpenGLTextureCubeH2M::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(m_Width, m_Height);
	}

	std::pair<uint32_t, uint32_t> OpenGLTextureCubeH2M::GetMipSize(uint32_t mip) const
	{
		Log::GetLogger()->error("OpenGLTextureCube::GetMipSize({0}) - method not implemented!", mip);
		return std::pair<uint32_t, uint32_t>();
	}

}
