#include "Renderbuffer.h"

#include <stdio.h>


Renderbuffer::Renderbuffer() : Attachment()
{
}

Renderbuffer::Renderbuffer(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat, unsigned int orderID)
	: Attachment::Attachment(width, height, AttachmentType::Renderbuffer, attachmentFormat, orderID)
{
	GLenum internalFormat;
	GLenum attachment;

	if (attachmentFormat == AttachmentFormat::Depth)
	{
		internalFormat = GL_DEPTH_COMPONENT;
		attachment = GL_DEPTH_ATTACHMENT;
	}
	else if (attachmentFormat == AttachmentFormat::Depth_24)
	{
		internalFormat = GL_DEPTH_COMPONENT24;
		attachment = GL_DEPTH_ATTACHMENT;
	}
	else if (attachmentFormat == AttachmentFormat::Depth_24_Stencil_8)
	{
		internalFormat = GL_DEPTH24_STENCIL8;
		attachment = GL_DEPTH_STENCIL_ATTACHMENT;
	}

	glGenRenderbuffers(1, &m_ID);
	Bind();
	// Create a depth and stencil renderbuffer object
	glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, m_Width, m_Height);
	// Attach the renderbuffer object
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, m_ID);
	Unbind();
	printf("Renderbuffer ID=%d, m_Width=%d, m_Height=%d\n", m_ID, m_Width, m_Height);
}

void Renderbuffer::Bind(unsigned int slot)
{
	glBindRenderbuffer(GL_RENDERBUFFER, m_ID);
}

void Renderbuffer::Unbind()
{
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

Renderbuffer::~Renderbuffer()
{
	glDeleteRenderbuffers(1, &m_ID);
}
