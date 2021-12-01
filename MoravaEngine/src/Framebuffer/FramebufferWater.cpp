#include "Framebuffer/FramebufferWater.h"

#include "Core/Log.h"

#include "GL/glew.h"

#include <cstdio>


FramebufferWater::FramebufferWater()
{
	m_FBO = 0;
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

	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	Log::GetLogger()->info("FramebufferWater FBO={0} {1}x{2}", m_FBO, m_Width, m_Height);
}

void FramebufferWater::AddColorAttachment(H2M::RefH2M<FramebufferTexture> colorAttachment)
{
	m_ColorAttachment = colorAttachment;
}

void FramebufferWater::AddDepthAttachment(H2M::RefH2M<FramebufferTexture> depthAttachment)
{
	m_DepthAttachment = depthAttachment;
}

void FramebufferWater::AddDepthBuffer(H2M::RefH2M<Renderbuffer> depthBuffer)
{
	m_DepthBuffer = depthBuffer;
}

void FramebufferWater::Bind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
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
	glDeleteFramebuffers(1, &m_FBO);
}
