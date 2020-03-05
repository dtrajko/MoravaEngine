#include "Texture.h"

#include "CommonValues.h"



Texture::Texture()
{
	textureID = 0;
	m_Width = 0;
	m_Height = 0;
	m_BitDepth = 0;
	fileLocation = "";
}

Texture::Texture(const char* fileLoc)
	: Texture()
{
	fileLocation = fileLoc;
}

bool Texture::LoadTexture(bool flipVert)
{
	stbi_set_flip_vertically_on_load(flipVert ? 1 : 0);
	m_Buffer = stbi_load(fileLocation, &m_Width, &m_Height, &m_BitDepth, 0);
	if (!m_Buffer)
	{
		printf("Failed to find: '%s'\n", fileLocation);
		return false;
	}

	GLenum internalFormat = 0;
	GLenum dataFormat = 0;
	if (m_BitDepth == 4)
	{
		internalFormat = GL_RGBA8;
		dataFormat = GL_RGBA;
	}
	else if (m_BitDepth == 3)
	{
		internalFormat = GL_RGB8;
		dataFormat = GL_RGB;
	}

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, m_Buffer);
	glGenerateMipmap(GL_TEXTURE_2D);

	printf("Loading texture '%s'\n", fileLocation);

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

int Texture::getRed(int x, int z)
{
	return (int)m_Buffer[((z * m_Width + x) * m_BitDepth) + 0];
}

int Texture::getGreen(int x, int z)
{
	return (int)m_Buffer[((z * m_Width + x) * m_BitDepth) + 1];
}

int Texture::getBlue(int x, int z)
{
	return (int)m_Buffer[((z * m_Width + x) * m_BitDepth) + 2];

}

int Texture::getAlpha(int x, int z)
{
	return (int)m_Buffer[((z * m_Width + x) * m_BitDepth) + 3];
}

void Texture::Bind(unsigned int textureUnit)
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureID);
	// printf("Bind texture ID=%d to slot=%d\n", textureID, textureUnit);
}

void Texture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::ClearTexture()
{
	stbi_image_free(m_Buffer);
	glDeleteTextures(1, &textureID);
	textureID = 0;
	m_Width = 0;
	m_Height = 0;
	m_BitDepth = 0;
	fileLocation = "";
}

Texture::~Texture()
{
	ClearTexture();
}
