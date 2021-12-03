#include "OpenGLMoravaTexture.h"

#include "H2M/Core/BaseH2M.h"

#include "Core/Log.h"

#include <fstream>
#include <exception>
#include <string>


OpenGLMoravaTexture::OpenGLMoravaTexture()
{
	m_Spec.Width = 0;
	m_Spec.Height = 0;
	m_Spec.BitDepth = 0;
	m_Spec.InternalFormat = 0;
	m_Spec.Border = 0;
	m_Spec.Format = 0;
	m_Spec.Type = GL_UNSIGNED_BYTE;
	m_Spec.FlipVertically = false;
	m_Spec.IsSampler = false;
	m_Spec.Texture_Wrap_S = GL_REPEAT;
	m_Spec.Texture_Wrap_T = GL_REPEAT;
	m_Spec.Texture_Wrap_R = GL_REPEAT;
	m_Spec.Texture_Min_Filter = GL_LINEAR;
	m_Spec.Texture_Mag_Filter = GL_LINEAR;
	m_Spec.MipLevel = 0;

	m_Level = 0;
	m_ID = 0;
	m_FileLocation = "";
	m_Buffer = nullptr;
	m_Format = H2M::ImageFormatH2M::RGBA;
}

OpenGLMoravaTexture::OpenGLMoravaTexture(const char* fileLoc, bool flipVert, bool isSampler, int filter)
	: OpenGLMoravaTexture()
{
	m_FileLocation = fileLoc;

	m_Spec.Texture_Min_Filter = filter;
	m_Spec.Texture_Mag_Filter = filter;
	m_Spec.FlipVertically = flipVert;
	m_Spec.IsSampler = isSampler;

	try {
		Load(m_Spec.FlipVertically);
	}
	// catch (const std::exception& e) {
	catch (...) {
		Log::GetLogger()->error("Failed to load a texture '{0}'!", m_FileLocation);
		// throw std::runtime_error(e.what());
	}
}

OpenGLMoravaTexture::OpenGLMoravaTexture(const char* fileLoc, uint32_t width, uint32_t height, bool isSampler, int filter)
	: OpenGLMoravaTexture()
{
	m_FileLocation = fileLoc;
	m_Spec.Width = width;
	m_Spec.Height = height;
	m_Spec.IsSampler = isSampler;
	m_Spec.BitDepth = 4;
	m_Spec.Texture_Min_Filter = filter;
	m_Spec.Texture_Mag_Filter = filter;

	m_Buffer = new unsigned char[m_Spec.Width * m_Spec.Height * m_Spec.BitDepth];

	if (!m_Spec.IsSampler)
	{
		stbi_image_free(m_Buffer);
	}
}

/**
* Constructor for a fully customizable 2D texture
*/
OpenGLMoravaTexture::OpenGLMoravaTexture(const char* fileLoc, Specification spec)
	: OpenGLMoravaTexture()
{
	m_FileLocation = fileLoc;

	m_Spec.InternalFormat     = spec.InternalFormat;
	m_Spec.Width              = spec.Width;
	m_Spec.Height             = spec.Height;
	m_Spec.Border             = spec.Border;
	m_Spec.Format             = spec.Format;
	m_Spec.Type               = spec.Type;
	m_Spec.Texture_Wrap_S     = spec.Texture_Wrap_S;
	m_Spec.Texture_Wrap_T     = spec.Texture_Wrap_T;
	m_Spec.Texture_Wrap_R     = spec.Texture_Wrap_R;
	m_Spec.Texture_Min_Filter = spec.Texture_Min_Filter;
	m_Spec.Texture_Mag_Filter = spec.Texture_Mag_Filter;
	m_Spec.MipLevel           = spec.MipLevel;
	m_Spec.FlipVertically     = spec.FlipVertically;
	m_Spec.BitDepth           = spec.BitDepth;
	m_Spec.IsSampler          = spec.IsSampler;
	m_Spec.Samples            = spec.Samples;
	m_Spec.IsMultisample      = spec.IsMultisample;
	m_Spec.IsSRGB             = spec.IsSRGB;

	Load(m_Spec.FlipVertically);
}

bool OpenGLMoravaTexture::Load(bool flipVert)
{
	if (m_Buffer)
	{
		Log::GetLogger()->info("Texture '{0}' already loaded. Skipping...", m_FileLocation);
		return true;
	}

	stbi_set_flip_vertically_on_load(flipVert ? 1 : 0);

	if (stbi_is_hdr(m_FileLocation))
	{
		Log::GetLogger()->info("Loading an HDR texture '{0}'", m_FileLocation);
		m_Buffer = (byte*)stbi_loadf(m_FileLocation, (int*)&m_Spec.Width, (int*)&m_Spec.Height, &m_Spec.BitDepth, 0);
		m_Format = H2M::ImageFormatH2M::RGBA16F;
	}
	else
	{
		m_Buffer = stbi_load(m_FileLocation, (int*)&m_Spec.Width, (int*)&m_Spec.Height, &m_Spec.BitDepth, 0);
		m_Format = H2M::ImageFormatH2M::RGBA;
	}

	if (!m_Buffer)
	{
		std::string message = "ERROR: Texture failed to load '" + std::string(m_FileLocation) + "'";
		throw std::runtime_error(message.c_str());
	}

	CreateAPISpecific();

	float fileSize = GetFileSize(m_FileLocation) / (1024.0f * 1024.0f);
	Log::GetLogger()->info("Loading texture '{0}' [ID={1}, size={2} MB]", m_FileLocation, m_ID, fileSize);

	if (!m_Spec.IsSampler)
	{
		stbi_image_free(m_Buffer);
	}

	return true;
}

void OpenGLMoravaTexture::CreateAPISpecific()
{
	// if InternalFormat and Format not explicitly specified, deduce from the BitDepth value
	if ((!m_Spec.InternalFormat || !m_Spec.Format) && m_Spec.BitDepth)
	{
		switch (m_Spec.BitDepth)
		{
		case 1:
			m_Spec.InternalFormat = GL_RED;
			m_Spec.Format = GL_RED;
			break;
		case 3:
			m_Spec.InternalFormat = GL_RGB8;
			m_Spec.Format = GL_RGB;
			break;
		case 4:
			m_Spec.InternalFormat = GL_RGBA8;
			m_Spec.Format = GL_RGBA;
			break;
		}
	}

	if (m_Spec.IsSRGB)
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &m_ID);
		glBindTexture(GL_TEXTURE_2D, m_ID);

		int mipLevels = CalculateMipMapCount(m_Spec.Width, m_Spec.Height);
		Log::GetLogger()->info("Creating a SRGB texture mip levels: {0}", mipLevels);

		glTextureStorage2D(m_ID, mipLevels, GL_SRGB8, m_Spec.Width, m_Spec.Height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_Spec.Texture_Wrap_S);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_Spec.Texture_Wrap_T);
		glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : m_Spec.Texture_Min_Filter);
		glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, m_Spec.Texture_Mag_Filter);

		glTextureSubImage2D(m_ID, 0, 0, 0, m_Spec.Width, m_Spec.Height, m_Spec.Format, m_Spec.Type, m_Buffer);
		glGenerateTextureMipmap(m_ID);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &m_ID);
		glBindTexture(GL_TEXTURE_2D, m_ID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_Spec.Texture_Wrap_S);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_Spec.Texture_Wrap_T);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_Spec.Texture_Min_Filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_Spec.Texture_Mag_Filter);

		glTexImage2D(GL_TEXTURE_2D, m_Level, m_Spec.InternalFormat, m_Spec.Width, m_Spec.Height, m_Spec.Border, m_Spec.Format, m_Spec.Type, m_Buffer);
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void OpenGLMoravaTexture::Save()
{
	CreateAPISpecific();
	stbi_write_png(m_FileLocation, m_Spec.Width, m_Spec.Height, m_Spec.BitDepth, m_Buffer, m_Spec.Width * m_Spec.BitDepth);
}

int OpenGLMoravaTexture::GetRed(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 0];
}

int OpenGLMoravaTexture::GetGreen(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 1];
}

int OpenGLMoravaTexture::GetBlue(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 2];

}

int OpenGLMoravaTexture::GetAlpha(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 3];
}

void OpenGLMoravaTexture::SetPixel(int x, int z, glm::ivec4 pixel)
{
	SetRed(  x, z, pixel.x);
	SetGreen(x, z, pixel.y);
	SetBlue( x, z, pixel.z);
	SetAlpha(x, z, pixel.w);
}

void OpenGLMoravaTexture::SetRed(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 0] = value;
}

void OpenGLMoravaTexture::SetGreen(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 1] = value;
}

void OpenGLMoravaTexture::SetBlue(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 2] = value;
}

void OpenGLMoravaTexture::SetAlpha(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 3] = value;
}

void OpenGLMoravaTexture::Bind(uint32_t textureSlot) const
{
	glActiveTexture(GL_TEXTURE0 + textureSlot);
	glBindTexture(GL_TEXTURE_2D, m_ID);
}

void OpenGLMoravaTexture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

float OpenGLMoravaTexture::GetFileSize(const char* filename)
{
	struct stat stat_buf;
	int rc = stat(filename, &stat_buf);
	return rc == 0 ? (float) stat_buf.st_size : -1.0f;
}

void OpenGLMoravaTexture::Clear()
{
	if (m_Spec.IsSampler)
		stbi_image_free(m_Buffer);
	glDeleteTextures(1, &m_ID);

	m_ID = 0;
	m_Spec.Width = 0;
	m_Spec.Height = 0;
	m_Spec.BitDepth = 0;
	m_FileLocation = "";
}

uint32_t OpenGLMoravaTexture::CalculateMipMapCount(uint32_t width, uint32_t height)
{
	uint32_t levels = 1;
	while ((width | height) >> levels)
		levels++;

	return levels;
}

uint32_t OpenGLMoravaTexture::GetMipLevelCount()
{
	return CalculateMipMapCount(m_Spec.Width, m_Spec.Height);
}

std::pair<uint32_t, uint32_t> OpenGLMoravaTexture::GetMipSize(uint32_t mip) const
{
	Log::GetLogger()->error("OpenGLMoravaTexture::GetMipSize({0}) - method not implemented!", mip);
	return std::pair<uint32_t, uint32_t>();
}

OpenGLMoravaTexture::~OpenGLMoravaTexture()
{
	Clear();
}
