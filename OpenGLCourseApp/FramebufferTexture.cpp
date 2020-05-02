#include "FramebufferTexture.h"

#include <GL/glew.h>

#include <stdexcept>


FramebufferTexture::FramebufferTexture()
	: Attachment()
{
}

FramebufferTexture::FramebufferTexture(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat, unsigned int orderID)
	: Attachment(width, height, AttachmentType::Texture, attachmentFormat, orderID)
{
	GLenum attachment;
	GLint internalFormat;
	GLenum format;
	GLenum type;

	switch (attachmentFormat)
	{
	case AttachmentFormat::Color:
		attachment = GL_COLOR_ATTACHMENT0 + orderID;
		internalFormat = GL_RGB;
		format = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		break;
	case AttachmentFormat::Depth:
		attachment = GL_DEPTH_ATTACHMENT;
		internalFormat = GL_DEPTH_COMPONENT32;
		format = GL_DEPTH_COMPONENT;
		type = GL_FLOAT;
		break;
	case AttachmentFormat::Stencil:
		attachment = GL_STENCIL_ATTACHMENT;
		internalFormat = GL_STENCIL_INDEX;
		format = GL_STENCIL_INDEX;
		type = GL_FLOAT;
		break;
	case AttachmentFormat::DepthStencil:
		attachment = GL_DEPTH_STENCIL_ATTACHMENT;
		internalFormat = GL_DEPTH24_STENCIL8;
		format = GL_DEPTH_STENCIL;
		type = GL_UNSIGNED_INT_24_8;
		break;
	default:
		throw std::runtime_error("AttachmentFormat not supported!");
		return;
	}

	glGenTextures(1, &m_ID);
	glBindTexture(GL_TEXTURE_2D, m_ID);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, type, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, attachment, m_ID, 0);
	// glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, m_ID, 0);
}

void FramebufferTexture::Bind(unsigned int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_ID);
}

void FramebufferTexture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

FramebufferTexture::~FramebufferTexture()
{
	glDeleteTextures(1, &m_ID);
}
