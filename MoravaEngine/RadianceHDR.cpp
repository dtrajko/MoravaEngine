#include "RadianceHDR.h"

#include "CommonValues.h"



RadianceHDR::RadianceHDR()
	: Texture()
{
	m_Spec.Texture_Wrap_S = GL_CLAMP_TO_EDGE;
	m_Spec.Texture_Wrap_T = GL_CLAMP_TO_EDGE;
	m_Spec.Texture_Min_Filter = GL_LINEAR;
	m_Spec.Texture_Mag_Filter = GL_LINEAR;
}

RadianceHDR::RadianceHDR(const char* fileLoc)
	: RadianceHDR()
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
	m_Spec.Format = GL_RGB;

	glGenTextures(1, &m_ID);
	glBindTexture(GL_TEXTURE_2D, m_ID);

	glTexImage2D(GL_TEXTURE_2D, 0, m_Spec.InternalFormat, m_Spec.Width, m_Spec.Height, 0, m_Spec.Format, GL_FLOAT, m_Buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_Spec.Texture_Wrap_S);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_Spec.Texture_Wrap_T);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_Spec.Texture_Min_Filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_Spec.Texture_Mag_Filter);

	// stbi_image_free(m_Buffer);

	printf("Loading texture '%s' [ID=%d]\n", m_FileLocation, m_ID);

	return true;
}

RadianceHDR::~RadianceHDR()
{
	Texture::Clear();
}
