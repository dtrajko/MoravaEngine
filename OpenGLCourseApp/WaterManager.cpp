#include "WaterManager.h"


WaterManager::WaterManager()
{
}

WaterManager::WaterManager(int width, int height)
{
	m_Width = width;
	m_Height = height;

	reflectionFB = new Framebuffer(m_Width, m_Height);

	FramebufferTexture* reflectionTextureAttachment = new FramebufferTexture(m_Width, m_Height, "color");
	reflectionFB->AddColorAttachment(reflectionTextureAttachment);

	Renderbuffer* reflectionDepthBuffer = new Renderbuffer(m_Width, m_Height);
	reflectionFB->AddDepthBuffer(reflectionDepthBuffer);

	SwitchToDefaultFramebuffer();

	refractionFB = new Framebuffer(m_Width, m_Height);

	FramebufferTexture* refractionTextureAttachment = new FramebufferTexture(m_Width, m_Height, "color");
	refractionFB->AddColorAttachment(refractionTextureAttachment);

	FramebufferTexture* refractionDepthAttachment = new FramebufferTexture(m_Width, m_Height, "depth");
	refractionFB->AddDepthAttachment(refractionDepthAttachment);

	SwitchToDefaultFramebuffer();
}

void WaterManager::SwitchToDefaultFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_Width, m_Height);
}

WaterManager::~WaterManager()
{
	delete reflectionFB;
	delete refractionFB;
}
