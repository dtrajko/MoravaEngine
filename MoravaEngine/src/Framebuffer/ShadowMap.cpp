#include "ShadowMap.h"

#include "Core/Log.h"


ShadowMap::ShadowMap()
{
	m_FBO = 0;
	m_TextureID = 0;
}

bool ShadowMap::Init(GLuint width, GLuint height)
{
	m_ShadowWidth = width;
	m_ShadowHeight = height;

	glGenFramebuffers(1, &m_FBO);
	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_ShadowWidth, m_ShadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float bColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_TextureID, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		Log::GetLogger()->error("Framebuffer Error: {0}", status);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Log::GetLogger()->info("ShadowMap succesfully created! [m_TextureID={0}]", m_TextureID);

	return true;
}

void ShadowMap::BindForWriting()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
}

void ShadowMap::ReadTexture(unsigned int textureUnit)
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

void ShadowMap::Unbind(unsigned int width, unsigned int height)
{
	// unbind custom framebuffer and make the default framebuffer active
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
}

ShadowMap::~ShadowMap()
{
	if (m_FBO)
	{
		glDeleteFramebuffers(1, &m_FBO);
	}

	if (m_TextureID)
	{
		glDeleteTextures(1, &m_TextureID);
	}
}
