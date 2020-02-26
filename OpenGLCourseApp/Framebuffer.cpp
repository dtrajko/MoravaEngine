#include "Framebuffer.h"

#include "GL/glew.h"

#include <cstdio>


Framebuffer::Framebuffer()
{
	fboID = 0;
	m_Width = 0;
	m_Height = 0;
}

Framebuffer::Framebuffer(int width, int height)
{
	m_Width = width;
	m_Height = height;

	glGenFramebuffers(1, &fboID);
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	textureID = 0;
}

bool Framebuffer::Init()
{
	glGenFramebuffers(1, &fboID);
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float bColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureID, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Framebuffer Error: %i\n", status);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

void Framebuffer::Write()
{
	glBindTexture(GL_TEXTURE_2D, 0); // to make sure the texture is not bound
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glViewport(0, 0, m_Width, m_Height);
}

void Framebuffer::Read(unsigned int textureUnit)
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureID);
}


Framebuffer::~Framebuffer()
{
	if (fboID)
	{
		glDeleteFramebuffers(1, &fboID);
	}

	if (textureID)
	{
		glDeleteTextures(1, &textureID);
	}
}
