#include "Framebuffer.h"

#include <GL/glew.h>

#include <stdexcept>


Framebuffer::Framebuffer()
{
	m_TextureAttachmentsColor = std::vector<FramebufferTexture*>();
	m_AttachmentDepth = nullptr;
	m_AttachmentStencil = nullptr;
	m_AttachmentDepthAndStencil = nullptr;
}

Framebuffer::Framebuffer(unsigned int width, unsigned int height)
	: Framebuffer()
{
	glGenFramebuffers(1, &m_FBO);
	Bind(width, height);
}

void Framebuffer::Bind(unsigned int width, unsigned int height)
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glViewport(0, 0, width, height);
}

void Framebuffer::Unbind(unsigned int width, unsigned int height)
{
	// unbind custom framebuffer and make the default framebuffer active
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
}

bool Framebuffer::CheckStatus()
{
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void Framebuffer::CreateTextureAttachmentColor(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat)
{
	FramebufferTexture* texture = new FramebufferTexture(width, height, attachmentFormat, (unsigned int)m_TextureAttachmentsColor.size());
	m_TextureAttachmentsColor.push_back(texture);
}

void Framebuffer::CreateAttachmentDepth(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (attachmentType == AttachmentType::Texture)
		m_AttachmentDepth = new FramebufferTexture(width, height, attachmentFormat, 0);
	else if (attachmentType == AttachmentType::Renderbuffer)
		m_AttachmentDepth = new Renderbuffer(width, height, attachmentFormat, 0);
}

void Framebuffer::CreateAttachmentStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (attachmentType == AttachmentType::Texture)
		m_AttachmentStencil = new FramebufferTexture(width, height, attachmentFormat, 0);
	else if (attachmentType == AttachmentType::Renderbuffer)
		m_AttachmentStencil = new Renderbuffer(width, height, attachmentFormat, 0);
}

void Framebuffer::CreateAttachmentDepthAndStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (attachmentType == AttachmentType::Texture)
		m_AttachmentDepthAndStencil = new FramebufferTexture(width, height, attachmentFormat, 0);
	else if (attachmentType == AttachmentType::Renderbuffer)
		m_AttachmentDepthAndStencil = new Renderbuffer(width, height, attachmentFormat, 0);
}

FramebufferTexture* Framebuffer::GetTextureAttachmentColor(unsigned int orderID)
{
	if (m_TextureAttachmentsColor.size() < (size_t)orderID + 1)
	{
		throw std::runtime_error("Color texture attachment does not exist [orderID = " + std::to_string(orderID) +
			", m_FBO = " + std::to_string(m_FBO) + "]");
	}

	return m_TextureAttachmentsColor.at(orderID);
}

Attachment* Framebuffer::GetAttachmentDepth()
{
	if (m_AttachmentDepth == nullptr)
	{
		throw std::runtime_error("Depth attachment does not exist in current Framebuffer [ " + std::to_string(m_FBO) + " ]");
	}

	return m_AttachmentDepth;
}

Attachment* Framebuffer::GetAttachmentStencil()
{
	if (m_AttachmentStencil == nullptr)
	{
		throw std::runtime_error("Stencil attachment does not exist in current Framebuffer [ " + std::to_string(m_FBO) + " ]");
	}
	return m_AttachmentStencil;
}

Attachment* Framebuffer::GetAttachmentDepthAndStencil()
{
	if (m_AttachmentDepthAndStencil == nullptr)
	{
		throw std::runtime_error("Depth/Stencil attachment does not exist in current Framebuffer [ " + std::to_string(m_FBO) + " ]");
	}

	return m_AttachmentDepthAndStencil;
}

void Framebuffer::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

Framebuffer::~Framebuffer()
{
	for (auto& textureAttachment : m_TextureAttachmentsColor)
		delete textureAttachment;

	m_TextureAttachmentsColor.clear();

	delete m_AttachmentDepth;
	delete m_AttachmentStencil;
	delete m_AttachmentDepthAndStencil;

	glDeleteFramebuffers(1, &m_FBO);
}
