#include "Framebuffer.h"

#include "GL/glew.h"

#include <cstdio>


Framebuffer::Framebuffer()
{
	fbo = 0;
	m_Width = 0;
	m_Height = 0;
	m_ColorAttachment = nullptr;
	m_DepthAttachment = nullptr;
	m_DepthBuffer = nullptr;
}

Framebuffer::Framebuffer(unsigned int width, unsigned int height)
{
	m_Width = width;
	m_Height = height;

	m_ColorAttachment = nullptr;
	m_DepthAttachment = nullptr;
	m_DepthBuffer = nullptr;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	printf("Framebuffer fbo=%d, m_Width=%d, m_Height=%d\n", fbo, m_Width, m_Height);
}

void Framebuffer::AddColorAttachment(FramebufferTexture* colorAttachment)
{
	m_ColorAttachment = colorAttachment;
}

void Framebuffer::AddDepthAttachment(FramebufferTexture* depthAttachment)
{
	m_DepthAttachment = depthAttachment;
}

void Framebuffer::AddDepthBuffer(Renderbuffer* depthBuffer)
{
	m_DepthBuffer = depthBuffer;
}

void Framebuffer::Bind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, m_Width, m_Height);
}

void Framebuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_ColorAttachment->Unbind();
	m_DepthAttachment->Unbind();
	glViewport(0, 0, m_Width, m_Height);
}

Framebuffer::~Framebuffer()
{
	if (m_DepthAttachment)
		delete m_DepthAttachment;

	if (m_DepthAttachment)
		delete m_DepthAttachment;

	if (m_DepthBuffer)
		delete m_DepthBuffer;

	if (fbo)
		glDeleteFramebuffers(1, &fbo);
}
