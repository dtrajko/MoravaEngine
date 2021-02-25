#include "WaterManager.h"

#include "../../Renderbuffer.h"


float WaterManager::m_WaveSpeed = 0.005f;

WaterManager::WaterManager()
{
	m_Width = 0;
	m_Height = 0;
	m_WaterHeight = 0.0f;
	m_MoveFactor = m_WaveSpeed;
}

WaterManager::WaterManager(int width, int height, float waterHeight, float waveSpeed)
{
	m_Width = width;
	m_Height = height;
	m_WaterHeight = waterHeight;
	m_WaveSpeed = waveSpeed;
	m_MoveFactor = waveSpeed;

	bool isMultisample = false;

	m_WaterColor = glm::vec4(0.0, 0.6, 1.0, 1.0);

	reflectionFB = new FramebufferWater(m_Width, m_Height);

	FramebufferTexture* reflectionTextureAttachment = new FramebufferTexture(m_Width, m_Height, isMultisample, AttachmentFormat::Color, 0);
	reflectionFB->AddColorAttachment(reflectionTextureAttachment);

	Renderbuffer* reflectionDepthBuffer = new Renderbuffer(m_Width, m_Height, AttachmentFormat::Depth, 0);
	reflectionFB->AddDepthBuffer(reflectionDepthBuffer);

	SwitchToDefaultFramebuffer();

	refractionFB = new FramebufferWater(m_Width, m_Height);

	FramebufferTexture* refractionTextureAttachment = new FramebufferTexture(m_Width, m_Height, isMultisample, AttachmentFormat::Color, 0);
	refractionFB->AddColorAttachment(refractionTextureAttachment);

	FramebufferTexture* refractionDepthAttachment = new FramebufferTexture(m_Width, m_Height, isMultisample, AttachmentFormat::Depth, 0);
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
