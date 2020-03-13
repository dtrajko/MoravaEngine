#include "TextureCubemap.h"

#include "CommonValues.h"



TextureCubemap::TextureCubemap()
	: Texture()
{
}

TextureCubemap::TextureCubemap(unsigned int width, unsigned int height)
{
	m_Width = width;
	m_Height = height;

	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);

	for (unsigned int i = 0; i < 6; ++i)
	{
		// note that we store each face with 16-bit floating point values
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_FLOAT, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	printf("TextureCubemap GL_TEXTURE_CUBE_MAP m_TextureID=%d\n", m_TextureID);
}

void TextureCubemap::Bind(unsigned int textureUnit)
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);
}

TextureCubemap::~TextureCubemap()
{
}
