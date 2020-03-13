#include "Cubemap.h"


Cubemap::Cubemap()
{
	m_Width = 0;
	m_Height = 0;
}

Cubemap::Cubemap(int width, int height)
{
	m_Width = width;
	m_Height = height;

	m_CaptureFBO = new Framebuffer(m_Width, m_Height);
	m_CaptureRBO = new Renderbuffer(m_Width, m_Height, GL_DEPTH_COMPONENT24);

	m_TextureCubemap = new TextureCubemap(m_Width, m_Height);
}

Cubemap::~Cubemap()
{
	delete m_CaptureFBO;
	delete m_CaptureRBO;
	delete m_TextureCubemap;
}
