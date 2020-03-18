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

	m_Buffer = stbi_load(m_FileLocation, (int*)&m_Width, (int*)&m_Height, &m_BitDepth, 0);

	if (!m_Buffer)
	{
		printf("Texture failed to load '%s'\n", m_FileLocation);
		return false;
	}

	GLenum internalFormat = GL_NONE;
	if (m_BitDepth == 1)
		internalFormat = GL_RED;
	else if (m_BitDepth == 3)
		internalFormat = GL_RGB;
	else if (m_BitDepth == 4)
		internalFormat = GL_RGBA;

	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, internalFormat, GL_UNSIGNED_BYTE, m_Buffer);
	glGenerateMipmap(GL_TEXTURE_2D);

	printf("Loading texture '%s' [ID=%d]\n", m_FileLocation, m_TextureID);

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

TextureJoey::~TextureJoey()
{
	Clear();
}
