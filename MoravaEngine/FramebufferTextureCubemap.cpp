#include "FramebufferTextureCubemap.h"

#include "Log.h"

#include <GL/glew.h>

#include <stdexcept>


FramebufferTextureCubemap::FramebufferTextureCubemap()
	: Attachment()
{
	m_Level = 0;
	m_Border = 0;
}

FramebufferTextureCubemap::FramebufferTextureCubemap(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat, unsigned int orderID)
	: Attachment(width, height, AttachmentType::Texture, attachmentFormat, orderID)
{
	m_Level = 0;
	m_Border = 0;

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
	case AttachmentFormat::RGBA16F:
		attachment = GL_COLOR_ATTACHMENT0 + orderID;
		internalFormat = GL_RGBA;
		format = GL_RGBA;
		type = GL_RGBA16F;
		break;
	case AttachmentFormat::RGBA8:
		attachment = GL_COLOR_ATTACHMENT0 + orderID;
		internalFormat = GL_RGBA;
		format = GL_RGBA;
		type = GL_RGBA8;
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
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);

	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_Level, internalFormat, m_Width, m_Height, m_Border, format, type, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT); // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	for (unsigned int i = 0; i < 6; ++i)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_ID, 0);
	}
}

void FramebufferTextureCubemap::Bind(unsigned int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);
}

void FramebufferTextureCubemap::Unbind()
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

FramebufferTextureCubemap::~FramebufferTextureCubemap()
{
	glDeleteTextures(1, &m_ID);
}
