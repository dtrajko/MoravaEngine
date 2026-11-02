#include "Framebuffer/Cubemap.h"


Cubemap::Cubemap()
{
	m_Width = 0;
	m_Height = 0;
}

Cubemap::Cubemap(int width, int height)
{
	m_Width = width;
	m_Height = height;

	printf("Create Environment Cubemap Framebuffer:\n");
	m_CaptureFBO = new FramebufferWater(m_Width, m_Height);
	printf("Create Environment Cubemap Renderbuffer:\n");
	m_CaptureRBO = H2M::RefH2M<Renderbuffer>::Create(m_Width, m_Height, AttachmentFormat::Depth_24, 0, m_CaptureFBO->GetID());

	m_TextureCubemap = new TextureCubemap(m_Width, m_Height);

	printf("Environment cubemap created.\n");
}

Cubemap::~Cubemap()
{
	delete m_CaptureFBO;
	delete m_TextureCubemap;
}
