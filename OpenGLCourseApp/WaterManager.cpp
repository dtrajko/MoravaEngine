#include "WaterManager.h"


float WaterManager::m_WaveSpeed = 0.005f;

WaterManager::WaterManager()
{
	m_Width = 0;
	m_Height = 0;
	m_WaterHeight = 0.0f;
	m_WaterMoveFactor = m_WaveSpeed;
}

WaterManager::WaterManager(int width, int height, float waterHeight, float waveSpeed)
{
	m_Width = width;
	m_Height = height;
	m_WaterHeight = waterHeight;
	m_WaveSpeed = waveSpeed;
	m_WaterMoveFactor = waveSpeed;

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
