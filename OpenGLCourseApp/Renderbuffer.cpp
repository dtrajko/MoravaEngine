#include "Renderbuffer.h"

#include <stdio.h>


Renderbuffer::Renderbuffer()
{
	bufferID = 0;
	m_Width = 0;
	m_Height = 0;
}

Renderbuffer::Renderbuffer(unsigned int m_Width, unsigned int m_Height, GLenum internalFormat)
{
	glGenRenderbuffers(1, &bufferID);
	glBindRenderbuffer(GL_RENDERBUFFER, bufferID);
	glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, m_Width, m_Height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, bufferID);
	printf("Renderbuffer ID=%d, m_Width=%d, m_Height=%d\n", bufferID, m_Width, m_Height);
}

Renderbuffer::~Renderbuffer()
{
}
