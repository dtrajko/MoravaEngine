#include "Framebuffer/Renderbuffer.h"

#include "Core/Log.h"
#include "Core/Util.h"

#include <stdio.h>
#include <stdexcept>


Renderbuffer::Renderbuffer() : Attachment()
{
}

Renderbuffer::Renderbuffer(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat, unsigned int orderID, uint32_t framebufferID)
	: Attachment(width, height, AttachmentType::Renderbuffer, attachmentFormat, orderID, framebufferID)
{
	GLenum internalFormat;
	GLenum attachment;

	switch (attachmentFormat)
	{
	case AttachmentFormat::Depth:
		internalFormat = GL_DEPTH_COMPONENT;
		attachment = GL_DEPTH_ATTACHMENT;
		break;
	case AttachmentFormat::Depth_24:
		internalFormat = GL_DEPTH_COMPONENT24;
		attachment = GL_DEPTH_ATTACHMENT;
		break;
	case AttachmentFormat::Stencil:
		internalFormat = GL_STENCIL_INDEX;
		attachment = GL_STENCIL_ATTACHMENT;
		break;
	case AttachmentFormat::DepthStencil:
	case AttachmentFormat::Depth_24_Stencil_8:
		internalFormat = GL_DEPTH24_STENCIL8;
		attachment = GL_DEPTH_STENCIL_ATTACHMENT;
		break;
	default:
		throw std::runtime_error("AttachmentFormat not supported!");
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);

	glGenRenderbuffers(1, &m_ID);
	glBindRenderbuffer(GL_RENDERBUFFER, m_ID);
	// Create a depth and stencil renderbuffer object
	glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, m_Width, m_Height);
	// Attach the renderbuffer object
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, m_ID);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// finally check if framebuffer is complete
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::GetLogger()->error("Renderbuffer::Renderbuffer: Framebuffer not complete! Status: '{0}', [{1}x{2}]", status, m_Width, m_Height);
	}

	Util::CheckOpenGLErrors("Renderbuffer::Renderbuffer");
}

void Renderbuffer::Bind(unsigned int slot) const
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
