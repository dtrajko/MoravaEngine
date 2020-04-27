#include "Framebuffer.h"

#include <GL/glew.h>


Framebuffer::Framebuffer()
{
	glGenFramebuffers(1, &m_FBO);
}

void Framebuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
}

void Framebuffer::Unbind()
{
	// unbind custom framebuffer and make the default framebuffer active
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Framebuffer::CheckStatus()
{
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void Framebuffer::AttachTexture(unsigned int width, unsigned int height, FramebufferTextureType type)
{
	FramebufferTexture* texture = new FramebufferTexture(width, height, type, (unsigned int)m_TextureAttachments.size());
	m_TextureAttachments.push_back(texture);
}

void Framebuffer::AttachRenderbuffer(unsigned int width, unsigned int height, RenderbufferFormatType internalFormat)
{
	Renderbuffer* renderbuffer = new Renderbuffer(width, height, internalFormat, (unsigned int)m_RenderbufferAttachments.size());
	m_RenderbufferAttachments.push_back(renderbuffer);
}

Framebuffer::~Framebuffer()
{
	glDeleteFramebuffers(1, &m_FBO);
}
