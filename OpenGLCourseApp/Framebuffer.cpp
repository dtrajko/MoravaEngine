#include "Framebuffer.h"

#include <GL/glew.h>


Framebuffer::Framebuffer()
{
	glGenFramebuffers(1, &m_FBO);

	m_TextureAttachmentsColor = std::vector<FramebufferTexture*>();
	m_TextureAttachmentDepth = nullptr;
	m_TextureAttachmentStencil = nullptr;

	m_BufferAttachmentDepth = nullptr;
	m_BufferAttachmentStencil = nullptr;
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

void Framebuffer::CreateTextureAttachmentColor(unsigned int width, unsigned int height, FramebufferTextureType txType)
{
	FramebufferTexture* texture = new FramebufferTexture(width, height, txType, (unsigned int)m_TextureAttachmentsColor.size());
	m_TextureAttachmentsColor.push_back(texture);
}

void Framebuffer::CreateTextureAttachmentDepth(unsigned int width, unsigned int height, FramebufferTextureType txType)
{
	m_TextureAttachmentDepth = new FramebufferTexture(width, height, txType, 0);
}

void Framebuffer::CreateTextureAttachmentStencil(unsigned int width, unsigned int height, FramebufferTextureType txType)
{
	m_TextureAttachmentStencil = new FramebufferTexture(width, height, txType, 0);
}

void Framebuffer::CreateBufferAttachmentDepth(unsigned int width, unsigned int height, RenderbufferFormatType formatType)
{
	m_BufferAttachmentDepth = new Renderbuffer(width, height, formatType, 0);
}

void Framebuffer::CreateBufferAttachmentStencil(unsigned int width, unsigned int height, RenderbufferFormatType formatType)
{
	m_BufferAttachmentStencil = new Renderbuffer(width, height, formatType, 0);
}

Framebuffer::~Framebuffer()
{
	glDeleteFramebuffers(1, &m_FBO);
}
