#include "FramebufferTexture.h"


FramebufferTexture::FramebufferTexture()
{
}

FramebufferTexture::FramebufferTexture(int width, int height, std::string txType)
{
	GLenum attachment = GL_COLOR_ATTACHMENT0;
	GLint internalFormat = GL_RGB;
	GLenum format = GL_RGB;
	GLenum type = GL_UNSIGNED_BYTE;

	if (txType == "color")
	{
		attachment = GL_COLOR_ATTACHMENT0;
		internalFormat = GL_RGB;
		format = GL_RGB;
		type = GL_UNSIGNED_BYTE;
	}
	else if (txType == "depth")
	{
		attachment = GL_DEPTH_ATTACHMENT;
		internalFormat = GL_DEPTH_COMPONENT32;
		format = GL_DEPTH_COMPONENT;
		type = GL_FLOAT;
	}

	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, attachment, m_TextureID, 0);
}

FramebufferTexture::~FramebufferTexture()
{
	glDeleteTextures(1, &m_TextureID);
}
