#include "Water/WaterManager.h"

#include "Framebuffer/Renderbuffer.h"


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

	m_ReflectionFB = new FramebufferWater(m_Width, m_Height);

	H2M::RefH2M<FramebufferTexture> reflectionTextureAttachment = H2M::RefH2M<FramebufferTexture>::Create(m_Width, m_Height, isMultisample, AttachmentFormat::Color, 0);
	m_ReflectionFB->AddColorAttachment(reflectionTextureAttachment);

	H2M::RefH2M<Renderbuffer> reflectionDepthBuffer = H2M::RefH2M<Renderbuffer>::Create(m_Width, m_Height, AttachmentFormat::Depth, 0, m_ReflectionFB->GetID());
	m_ReflectionFB->AddDepthBuffer(reflectionDepthBuffer);

	SwitchToDefaultFramebuffer();

	m_RefractionFB = new FramebufferWater(m_Width, m_Height);

	H2M::RefH2M<FramebufferTexture> refractionTextureAttachment = H2M::RefH2M<FramebufferTexture>::Create(m_Width, m_Height, isMultisample, AttachmentFormat::Color, 0);
	m_RefractionFB->AddColorAttachment(refractionTextureAttachment);

	H2M::RefH2M<FramebufferTexture> refractionDepthAttachment = H2M::RefH2M<FramebufferTexture>::Create(m_Width, m_Height, isMultisample, AttachmentFormat::Depth, 0);
	m_RefractionFB->AddDepthAttachment(refractionDepthAttachment);

	SwitchToDefaultFramebuffer();
}

void WaterManager::SwitchToDefaultFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_Width, m_Height);
}

WaterManager::~WaterManager()
{
	delete m_ReflectionFB;
	delete m_RefractionFB;
}
