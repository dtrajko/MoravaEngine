#include "RadianceHDR.h"

#include "CommonValues.h"



RadianceHDR::RadianceHDR()
	: Texture()
{
}

RadianceHDR::RadianceHDR(const char* fileLoc)
	: Texture()
{
	m_FileLocation = fileLoc;
}

bool RadianceHDR::Load()
{
	stbi_set_flip_vertically_on_load(true);
	m_Buffer = stbi_loadf(m_FileLocation, (int*)&m_Width, (int*)&m_Height, &m_BitDepth, 0);
	if (!m_Buffer)
	{
		printf("Failed to load HDR image: '%s'\n", m_FileLocation);
		return false;
	}

	GLenum internalFormat = GL_RGB16F;
	GLenum dataFormat = GL_RGB;

	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_FLOAT, m_Buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(m_Buffer);

	printf("Loading texture '%s'\n", m_FileLocation);

	return true;
}

RadianceHDR::~RadianceHDR()
{
}
