#include "Renderbuffer.h"

#include <stdio.h>


Renderbuffer::Renderbuffer()
{
	m_RBO = 0;
	m_Width = 0;
	m_Height = 0;
}

Renderbuffer::Renderbuffer(unsigned int width, unsigned int height, RBOType formatType, unsigned int orderID)
{
	GLenum internalFormat;
	GLenum attachment;

	if (formatType == RBOType::Depth)
	{
		internalFormat = GL_DEPTH_COMPONENT;
		attachment = GL_DEPTH_ATTACHMENT;
	}
	else if (formatType == RBOType::Depth_24)
	{
		internalFormat = GL_DEPTH_COMPONENT24;
		attachment = GL_DEPTH_ATTACHMENT;
	}
	else if (formatType == RBOType::Depth_24_Stencil_8)
	{
		internalFormat = GL_DEPTH24_STENCIL8;
		attachment = GL_DEPTH_STENCIL_ATTACHMENT;
	}

	glGenRenderbuffers(1, &m_RBO);
	Bind();
	// Create a depth and stencil renderbuffer object
	glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, m_Width, m_Height);
	// Attach the renderbuffer object
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, m_RBO);

	printf("Renderbuffer ID=%d, m_Width=%d, m_Height=%d\n", m_RBO, m_Width, m_Height);
}

void Renderbuffer::Bind()
{
	glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
}

void Renderbuffer::Unbind()
{
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

Renderbuffer::~Renderbuffer()
{
	glDeleteRenderbuffers(1, &m_RBO);
}
