#include "FramebufferWater.h"

#include "Log.h"

#include "GL/glew.h"

#include <cstdio>


FramebufferWater::FramebufferWater()
{
	fbo = 0;
	m_Width = 0;
	m_Height = 0;
	m_ColorAttachment = nullptr;
	m_DepthAttachment = nullptr;
	m_DepthBuffer = nullptr;
}

FramebufferWater::FramebufferWater(unsigned int width, unsigned int height)
{
	m_Width = width;
	m_Height = height;

	m_ColorAttachment = nullptr;
	m_DepthAttachment = nullptr;
	m_DepthBuffer = nullptr;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	Log::GetLogger()->info("FramebufferWater FBO={0}, m_Width={1}, m_Height={2}", fbo, m_Width, m_Height);

}

void FramebufferWater::AddColorAttachment(FramebufferTexture* colorAttachment)
{
	m_ColorAttachment = colorAttachment;
}

void FramebufferWater::AddDepthAttachment(FramebufferTexture* depthAttachment)
{
	m_DepthAttachment = depthAttachment;
}

void FramebufferWater::AddDepthBuffer(Renderbuffer* depthBuffer)
{
	m_DepthBuffer = depthBuffer;
}

void FramebufferWater::Bind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, m_Width, m_Height);
}

void FramebufferWater::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_ColorAttachment->Unbind();
	m_DepthAttachment->Unbind();
	glViewport(0, 0, m_Width, m_Height);
}

FramebufferWater::~FramebufferWater()
{
	delete m_ColorAttachment;
	delete m_DepthAttachment;
	delete m_DepthBuffer;
	glDeleteFramebuffers(1, &fbo);
}
