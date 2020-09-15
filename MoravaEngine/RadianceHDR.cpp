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
	m_Buffer = stbi_loadf(m_FileLocation, (int*)&m_Spec.Width, (int*)&m_Spec.Height, &m_Spec.BitDepth, 0);
	if (!m_Buffer)
	{
		printf("Failed to load HDR image: '%s'\n", m_FileLocation);
		return false;
	}

	m_Spec.InternalFormat = GL_RGB16F;
	m_Spec.DataFormat = GL_RGB;

	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	glTexImage2D(GL_TEXTURE_2D, 0, m_Spec.InternalFormat, m_Spec.Width, m_Spec.Height, 0, m_Spec.DataFormat, GL_FLOAT, m_Buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// stbi_image_free(m_Buffer);

	printf("Loading texture '%s' [ID=%d]\n", m_FileLocation, m_TextureID);

	return true;
}

RadianceHDR::~RadianceHDR()
{
	Texture::Clear();
}
