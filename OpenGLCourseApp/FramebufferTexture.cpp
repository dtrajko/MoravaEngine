#include "FramebufferTexture.h"


FramebufferTexture::FramebufferTexture()
{
}

FramebufferTexture::FramebufferTexture(unsigned int width, unsigned int height, FBOTextureType txType, unsigned int orderID)
{
	GLenum attachment = GL_COLOR_ATTACHMENT0 + orderID;
	GLint internalFormat = GL_RGB;
	GLenum format = GL_RGB;
	GLenum type = GL_UNSIGNED_BYTE;

	if (txType == FBOTextureType::Color)
	{
		attachment = GL_COLOR_ATTACHMENT0 + orderID;
		internalFormat = GL_RGB;
		format = GL_RGB;
		type = GL_UNSIGNED_BYTE;
	}
	else if (txType == FBOTextureType::Depth)
	{
		attachment = GL_DEPTH_ATTACHMENT;
		internalFormat = GL_DEPTH_COMPONENT;
		format = GL_DEPTH_COMPONENT;
		type = GL_FLOAT;
	}
	else if (txType == FBOTextureType::Stencil)
	{
		attachment = GL_STENCIL_ATTACHMENT;
		internalFormat = GL_STENCIL_INDEX;
		format = GL_STENCIL_INDEX;
		type = GL_FLOAT;
	}
	else if (txType == FBOTextureType::DepthStencil)
	{
		attachment = GL_DEPTH_STENCIL_ATTACHMENT;
		internalFormat = GL_DEPTH24_STENCIL8;
		format = GL_DEPTH_STENCIL;
		type = GL_UNSIGNED_INT_24_8;
	}

	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture(GL_FRAMEBUFFER, attachment, m_TextureID, 0);
}

FramebufferTexture::~FramebufferTexture()
{
	glDeleteTextures(1, &m_TextureID);
}
