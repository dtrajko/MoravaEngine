#include "GBufferSSAO.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include <GL/glew.h>


GBufferSSAO::GBufferSSAO()
{
	ResetHandlers();

	m_WidthPrev = 0;
	m_HeightPrev = 0;
}

GBufferSSAO::~GBufferSSAO()
{
	Release();
}

void GBufferSSAO::Init()
{
	GenerateConditional();
}

void GBufferSSAO::GenerateConditional()
{
	m_Width = Application::Get()->GetWindow()->GetWidth();
	m_Height = Application::Get()->GetWindow()->GetHeight();

	if (m_Width != m_WidthPrev || m_Height != m_HeightPrev)
	{
		Generate();

		m_WidthPrev = m_Width;
		m_HeightPrev = m_Height;
	}
}

void GBufferSSAO::Update()
{
	GenerateConditional();
}

void GBufferSSAO::Generate()
{
	Log::GetLogger()->warn("GBufferSSAO::Generate - width: {0}, height: {1}", m_Width, m_Height);

	Release();

	// configure g-buffer framebuffer
	// ------------------------------
	glGenFramebuffers(1, &m_GBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_GBuffer);

	// position color buffer
	glGenTextures(1, &m_GBufferPosition);
	glBindTexture(GL_TEXTURE_2D, m_GBufferPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_GBufferPosition, 0);

	// normal color buffer
	glGenTextures(1, &m_GBufferNormal);
	glBindTexture(GL_TEXTURE_2D, m_GBufferNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_GBufferNormal, 0);

	// color + specular color buffer
	glGenTextures(1, &m_GBufferAlbedo);
	glBindTexture(GL_TEXTURE_2D, m_GBufferAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_GBufferAlbedo, 0);

	// Texcoord buffer
	glGenTextures(1, &m_GBufferTexCoord);
	glBindTexture(GL_TEXTURE_2D, m_GBufferTexCoord);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_GBufferTexCoord, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_RBO_Depth);
	glBindRenderbuffer(GL_RENDERBUFFER, m_RBO_Depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_Width, m_Height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RBO_Depth);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		Log::GetLogger()->error("GBufferSSAO::Generate: Framebuffer not complete!");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBufferSSAO::BindForWriting()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_GBuffer);
}

void GBufferSSAO::BindForReading()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_GBuffer);
}

void GBufferSSAO::SetReadBuffer(uint32_t attachmentIndex)
{
	glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
}

void GBufferSSAO::Release()
{
	Log::GetLogger()->info("GBufferSSAO::Release");

	if (m_GBuffer)
		glDeleteFramebuffers(1, &m_GBuffer);

	if (m_GBufferPosition)
		glDeleteTextures(1, &m_GBufferPosition);
	if (m_GBufferNormal)
		glDeleteTextures(1, &m_GBufferNormal);
	if (m_GBufferAlbedo)
		glDeleteTextures(1, &m_GBufferAlbedo);
	if (m_GBufferTexCoord)
		glDeleteTextures(1, &m_GBufferTexCoord);

	if (m_RBO_Depth)
		glDeleteRenderbuffers(1, &m_RBO_Depth);

	ResetHandlers();
}

void GBufferSSAO::ResetHandlers()
{
	// Framebuffers
	m_GBuffer = 0;

	// textures / framebuffer attachments
	m_GBufferPosition = 0;
	m_GBufferNormal = 0;
	m_GBufferAlbedo = 0;
	m_GBufferTexCoord = 0;
	m_RBO_Depth = 0;
}
