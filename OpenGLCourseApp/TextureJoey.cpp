#include "TextureJoey.h"



TextureJoey::TextureJoey()
	: Texture()
{
}

TextureJoey::TextureJoey(const char* fileLoc, bool flipVert)
	: TextureJoey()
{
	m_FileLocation = fileLoc;

	Load(flipVert);
}

bool TextureJoey::Load(bool flipVert)
{
	if (m_Buffer)
	{
		printf("Texture '%s' already loaded. Skipping...\n", m_FileLocation);
		return true;
	}

	glGenTextures(1, &m_TextureID);

	m_Buffer = stbi_load(m_FileLocation, (int*)&m_Width, (int*)&m_Height, &m_BitDepth, 0);
	if (m_Buffer)
	{
		GLenum format = GL_NONE;
		if (m_BitDepth == 1)
			format = GL_RED;
		else if (m_BitDepth == 3)
			format = GL_RGB;
		else if (m_BitDepth == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, m_Buffer);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		printf("Texture failed to load at path '%s'\n", m_FileLocation);
	}

	return true;
}

TextureJoey::~TextureJoey()
{
	Clear();
}
