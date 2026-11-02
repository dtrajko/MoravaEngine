#include "ParticleTexture.h"

#include <GL/glew.h>


ParticleTexture::ParticleTexture()
{
}

ParticleTexture::ParticleTexture(int textureID, int numberOfRows)
{
	m_TextureID = textureID;
	m_NumberOfRows = numberOfRows <= 0 ? 1 : numberOfRows;
}

void ParticleTexture::Bind(unsigned int textureUnit)
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

ParticleTexture::~ParticleTexture()
{
}
