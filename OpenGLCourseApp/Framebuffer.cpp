#include "Framebuffer.h"

#include "GL/glew.h"

#include <cstdio>


Framebuffer::Framebuffer()
{
	fbo = 0;
	m_Width = 0;
	m_Height = 0;
}

Framebuffer::Framebuffer(unsigned int width, unsigned int height)
{
	m_Width = width;
	m_Height = height;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void Framebuffer::AddColorAttachment(FramebufferTexture* colorAttachment)
{
	m_ColorAttachments.push_back(colorAttachment);
}

void Framebuffer::AddDepthAttachment(FramebufferTexture* depthAttachment)
{
	m_DepthAttachment = depthAttachment;
}

void Framebuffer::AddDepthBuffer(Renderbuffer* depthBuffer)
{
	m_DepthBuffer = depthBuffer;
}

void Framebuffer::Write()
{
	glBindTexture(GL_TEXTURE_2D, 0); // to make sure the texture is not bound
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, m_Width, m_Height);
}

void Framebuffer::Read(unsigned int textureUnit)
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	// glBindTexture(GL_TEXTURE_2D, textureID);
}

void Framebuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_Width, m_Height);
}

Framebuffer::~Framebuffer()
{
	for (auto& colorAttachment : m_ColorAttachments)
		delete colorAttachment;

	if (m_DepthAttachment)
		delete m_DepthAttachment;

	if (m_DepthBuffer)
		delete m_DepthBuffer;

	if (fbo)
		glDeleteFramebuffers(1, &fbo);
}
