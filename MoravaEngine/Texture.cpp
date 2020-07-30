#include "Texture.h"
#include "Log.h"

#include <fstream>


Texture::Texture()
{
	m_TextureID = 0;
	m_Width = 0;
	m_Height = 0;
	m_BitDepth = 0;
	m_FileLocation = "";
	m_Filter = GL_LINEAR;
	m_IsSampler = false;
}

Texture::Texture(const char* fileLoc, bool flipVert)
	: Texture()
{
	m_FileLocation = fileLoc;
	m_Filter = GL_LINEAR;

	Load(flipVert);
}

Texture::Texture(const char* fileLoc, bool flipVert, bool isSampler)
	: Texture()
{
	m_FileLocation = fileLoc;
	m_Filter = GL_LINEAR;
	m_IsSampler = isSampler;

	Load(flipVert);
}

Texture::Texture(const char* fileLoc, unsigned int width, unsigned int height, bool isSampler, GLenum filter)
{
	m_FileLocation = fileLoc;
	m_Width = width;
	m_Height = height;
	m_IsSampler = isSampler;
	m_Filter = filter;
	m_BitDepth = 4;

	m_Buffer = new unsigned char[m_Width * m_Height * m_BitDepth];

	if (!m_IsSampler)
		stbi_image_free(m_Buffer);
}

Texture::Texture(const char* fileLoc, bool flipVert, GLenum filter)
	: Texture()
{
	m_FileLocation = fileLoc;
	m_Filter = filter;

	Load(flipVert);
}

bool Texture::Load(bool flipVert)
{
	if (m_Buffer)
	{
		Log::GetLogger()->info("Texture '{0}' already loaded. Skipping...", m_FileLocation);
		return true;
	}

	stbi_set_flip_vertically_on_load(flipVert ? 1 : 0);
	m_Buffer = stbi_load(m_FileLocation, (int*)&m_Width, (int*)&m_Height, &m_BitDepth, 0);
	if (!m_Buffer)
	{
		Log::GetLogger()->error("ERROR: Texture failed to load '{0}'", m_FileLocation);
		return false;
	}

	OpenGLCreate();

	float fileSize = GetFileSize(m_FileLocation) / (1024.0f * 1024.0f);
	Log::GetLogger()->info("Loading texture '{0}' [ID={1}, size={2} MB]", m_FileLocation, m_TextureID, fileSize);

	if (!m_IsSampler)
		stbi_image_free(m_Buffer);

	return true;
}

void Texture::OpenGLCreate()
{
	GLenum internalFormat = 0;
	GLenum dataFormat = 0;
	if (m_BitDepth == 1)
	{
		internalFormat = GL_RED;
		dataFormat = GL_RED;
	}
	else if (m_BitDepth == 3)
	{
		internalFormat = GL_RGB8;
		dataFormat = GL_RGB;
	}
	else if (m_BitDepth == 4)
	{
		internalFormat = GL_RGBA8;
		dataFormat = GL_RGBA;
	}

	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_Filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_Filter);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, m_Buffer);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Save()
{
	OpenGLCreate();
	stbi_write_png(m_FileLocation, m_Width, m_Height, m_BitDepth, m_Buffer, m_Width * m_BitDepth);
}

int Texture::GetRed(int x, int z)
{
	return (int)m_Buffer[((z * m_Width + x) * m_BitDepth) + 0];
}

int Texture::GetGreen(int x, int z)
{
	return (int)m_Buffer[((z * m_Width + x) * m_BitDepth) + 1];
}

int Texture::GetBlue(int x, int z)
{
	return (int)m_Buffer[((z * m_Width + x) * m_BitDepth) + 2];

}

int Texture::GetAlpha(int x, int z)
{
	return (int)m_Buffer[((z * m_Width + x) * m_BitDepth) + 3];
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
	m_Buffer[((z * m_Width + x) * m_BitDepth) + 0] = value;
}

void Texture::SetGreen(int x, int z, int value)
{
	m_Buffer[((z * m_Width + x) * m_BitDepth) + 1] = value;
}

void Texture::SetBlue(int x, int z, int value)
{
	m_Buffer[((z * m_Width + x) * m_BitDepth) + 2] = value;
}

void Texture::SetAlpha(int x, int z, int value)
{
	m_Buffer[((z * m_Width + x) * m_BitDepth) + 3] = value;
}

void Texture::Bind(unsigned int textureUnit)
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
	// printf("Texture: bind texture ID=%d to slot=%d\n", m_TextureID, textureUnit);
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
	if (m_IsSampler)
		stbi_image_free(m_Buffer);
	glDeleteTextures(1, &m_TextureID);
	m_TextureID = 0;
	m_Width = 0;
	m_Height = 0;
	m_BitDepth = 0;
	m_FileLocation = "";
}

Texture::~Texture()
{
	Clear();
}
