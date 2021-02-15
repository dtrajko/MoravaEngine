#include "Texture.h"
#include "Log.h"

#include <fstream>
#include <exception>
#include <string>


Texture::Texture()
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
	m_Format = TextureFormat::RGBA;
}

Texture::Texture(const char* fileLoc, bool flipVert)
	: Texture()
{
	m_FileLocation = fileLoc;
	m_Spec.FlipVertically = flipVert;

	try {
		Load(m_Spec.FlipVertically);
	}
	catch (const std::exception& e) {
		throw std::runtime_error(e.what());
	}
}

Texture::Texture(const char* fileLoc, bool flipVert, bool isSampler)
	: Texture()
{
	m_Spec.IsSampler = isSampler;
	m_Spec.FlipVertically = flipVert;

	m_FileLocation = fileLoc;

	try {
		Load(m_Spec.FlipVertically);
	}
	catch (const std::exception& e) {
		throw std::runtime_error(e.what());
	}
}

Texture::Texture(const char* fileLoc, unsigned int width, unsigned int height, bool isSampler, int filter)
	: Texture()
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
		stbi_image_free(m_Buffer);
}

/**
* Constructor for a fully customizable 2D texture
*/
Texture::Texture(const char* fileLoc, Specification spec)
	: Texture()
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

Texture::Texture(const char* fileLoc, bool flipVert, bool isSampler, int filter)
	: Texture()
{
	m_FileLocation = fileLoc;

	m_Spec.Texture_Min_Filter = filter;
	m_Spec.Texture_Mag_Filter = filter;
	m_Spec.FlipVertically = flipVert;
	m_Spec.IsSampler = isSampler;

	try {
		Load(m_Spec.FlipVertically);
	}
	catch(...) {
		Log::GetLogger()->error("Failed to load a texture '{0}'!", fileLoc);
	}
}

bool Texture::Load(bool flipVert)
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
		m_Format = TextureFormat::Float16;
	}
	else
	{
		m_Buffer = stbi_load(m_FileLocation, (int*)&m_Spec.Width, (int*)&m_Spec.Height, &m_Spec.BitDepth, 0);
		m_Format = TextureFormat::RGBA;
	}

	if (!m_Buffer)
	{
		std::string message = "ERROR: Texture failed to load '" + std::string(m_FileLocation) + "'";
		throw std::runtime_error(message.c_str());
	}

	OpenGLCreate();

	float fileSize = GetFileSize(m_FileLocation) / (1024.0f * 1024.0f);
	Log::GetLogger()->info("Loading texture '{0}' [ID={1}, size={2} MB]", m_FileLocation, m_ID, fileSize);

	if (!m_Spec.IsSampler)
		stbi_image_free(m_Buffer);

	return true;
}

void Texture::OpenGLCreate()
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

		int mipLevels = Texture::CalculateMipMapCount(m_Spec.Width, m_Spec.Height);
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

void Texture::Save()
{
	OpenGLCreate();
	stbi_write_png(m_FileLocation, m_Spec.Width, m_Spec.Height, m_Spec.BitDepth, m_Buffer, m_Spec.Width * m_Spec.BitDepth);
}

int Texture::GetRed(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 0];
}

int Texture::GetGreen(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 1];
}

int Texture::GetBlue(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 2];

}

int Texture::GetAlpha(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 3];
}

void Texture::SetPixel(int x, int z, glm::ivec4 pixel)
{
	SetRed(  x, z, pixel.x);
	SetGreen(x, z, pixel.y);
	SetBlue( x, z, pixel.z);
	SetAlpha(x, z, pixel.w);
}

void Texture::SetRed(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 0] = value;
}

void Texture::SetGreen(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 1] = value;
}

void Texture::SetBlue(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 2] = value;
}

void Texture::SetAlpha(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 3] = value;
}

void Texture::Bind(unsigned int textureSlot)
{
	glActiveTexture(GL_TEXTURE0 + textureSlot);
	glBindTexture(GL_TEXTURE_2D, m_ID);
}

void Texture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

float Texture::GetFileSize(const char* filename)
{
	struct stat stat_buf;
	int rc = stat(filename, &stat_buf);
	return rc == 0 ? (float) stat_buf.st_size : -1.0f;
}

void Texture::Clear()
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

unsigned int Texture::CalculateMipMapCount(unsigned int width, unsigned int height)
{
	unsigned int levels = 1;
	while ((width | height) >> levels)
		levels++;

	return levels;
}

unsigned int Texture::GetMipLevelCount()
{
	return CalculateMipMapCount(m_Spec.Width, m_Spec.Height);
}

Texture::~Texture()
{
	Clear();
}
