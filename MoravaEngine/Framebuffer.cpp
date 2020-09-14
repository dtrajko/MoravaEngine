#include "Framebuffer.h"
#include "Log.h"
#include "Application.h"

#include <GL/glew.h>

#include <stdexcept>


static const uint32_t s_MaxFramebufferSize = 8192;

Framebuffer::Framebuffer()
{
	m_FBO = 0;

	m_Width = Application::Get()->GetWindow()->GetBufferWidth();
	m_Height = Application::Get()->GetWindow()->GetBufferHeight();

	m_TextureAttachmentsColor = std::vector<FramebufferTexture*>();
	m_AttachmentDepth = nullptr;
	m_AttachmentStencil = nullptr;
	m_AttachmentDepthAndStencil = nullptr;
}

Framebuffer::Framebuffer(unsigned int width, unsigned int height)
	: Framebuffer()
{
	m_Width = width;
	m_Height = height;
}

void Framebuffer::AddAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	FramebufferSpecification fbSpecs;
	fbSpecs.Width = width;
	fbSpecs.Height = height;
	fbSpecs.attachmentType = attachmentType;
	fbSpecs.attachmentFormat = attachmentFormat;
	m_AttachmentSpecs.push_back(fbSpecs);
}

void Framebuffer::Generate(unsigned int width, unsigned int height)
{
	if (m_FBO) {
		Release();
	}

	m_Width = width;
	m_Height = height;

	glGenFramebuffers(1, &m_FBO);
	Bind(m_Width, m_Height);

	for (FramebufferSpecification attachmentSpecs : m_AttachmentSpecs) {
		// m_Width = attachmentSpecs.Width;
		// m_Height = attachmentSpecs.Height;

		switch (attachmentSpecs.attachmentFormat)
		{
		case AttachmentFormat::Color:
			CreateTextureAttachmentColor(m_Width, m_Height, attachmentSpecs.attachmentFormat);
			break;
		case AttachmentFormat::Depth:
			CreateAttachmentDepth(m_Width, m_Height, attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			break;
		case AttachmentFormat::Stencil:
			CreateAttachmentStencil(m_Width, m_Height, attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			break;
		case AttachmentFormat::DepthStencil:
			CreateAttachmentDepthAndStencil(m_Width, m_Height, attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			break;
		case AttachmentFormat::Depth_24_Stencil_8:
			CreateAttachmentDepthAndStencil(m_Width, m_Height, attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			break;
		case AttachmentFormat::RGBA16F:
			CreateAttachmentRGBA16F(m_Width, m_Height, attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			break;
		default:
			Log::GetLogger()->error("Attachment format '{0}' not supported.", attachmentSpecs.attachmentFormat);
			break;
		}
	}
}

void Framebuffer::Release()
{
	// Log::GetLogger()->info("Framebuffer::Release");

	for (auto& textureAttachment : m_TextureAttachmentsColor)
		delete textureAttachment;

	m_TextureAttachmentsColor.clear();

	delete m_AttachmentDepth;
	delete m_AttachmentStencil;
	delete m_AttachmentDepthAndStencil;

	glDeleteFramebuffers(1, &m_FBO);

	m_FBO = 0;
}

void Framebuffer::CreateTextureAttachmentColor(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat)
{
	FramebufferTexture* texture = new FramebufferTexture(width, height, attachmentFormat, (unsigned int)m_TextureAttachmentsColor.size());
	m_TextureAttachmentsColor.push_back(texture);

	Log::GetLogger()->info("Framebuffer::CreateTextureAttachmentColor [ID={0}, {1}x{2}]",
		texture->GetID(), texture->GetWidth(), texture->GetHeight());
}

void Framebuffer::CreateAttachmentDepth(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (attachmentType == AttachmentType::Texture)
		m_AttachmentDepth = new FramebufferTexture(width, height, attachmentFormat, 0);
	else if (attachmentType == AttachmentType::Renderbuffer)
		m_AttachmentDepth = new Renderbuffer(width, height, attachmentFormat, 0);
}

void Framebuffer::CreateAttachmentStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (attachmentType == AttachmentType::Texture)
		m_AttachmentStencil = new FramebufferTexture(width, height, attachmentFormat, 0);
	else if (attachmentType == AttachmentType::Renderbuffer)
		m_AttachmentStencil = new Renderbuffer(width, height, attachmentFormat, 0);
}

void Framebuffer::CreateAttachmentDepthAndStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (attachmentType == AttachmentType::Texture)
		m_AttachmentDepthAndStencil = new FramebufferTexture(width, height, attachmentFormat, 0);
	else if (attachmentType == AttachmentType::Renderbuffer)
		m_AttachmentDepthAndStencil = new Renderbuffer(width, height, attachmentFormat, 0);
}

void Framebuffer::CreateAttachmentRGBA16F(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	FramebufferTexture* texture = new FramebufferTexture(width, height, attachmentFormat, (unsigned int)m_TextureAttachmentsColor.size());
	m_TextureAttachmentsColor.push_back(texture);
}

void Framebuffer::CreateAttachmentRGBA8(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	FramebufferTexture* texture = new FramebufferTexture(width, height, attachmentFormat, (unsigned int)m_TextureAttachmentsColor.size());
	m_TextureAttachmentsColor.push_back(texture);
}

void Framebuffer::Bind()
{
	Bind(m_Width, m_Height);
}

void Framebuffer::Unbind()
{
	Unbind(m_Width, m_Height);
}

void Framebuffer::Bind(unsigned int width, unsigned int height)
{
	m_Width = width;
	m_Height = height;

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glViewport(0, 0, m_Width, m_Height);
}

void Framebuffer::Unbind(unsigned int width, unsigned int height)
{
	m_Width = width;
	m_Height = height;

	// unbind custom framebuffer and make the default framebuffer active
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_Width, m_Height);
}

bool Framebuffer::CheckStatus()
{
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

FramebufferTexture* Framebuffer::GetTextureAttachmentColor(unsigned int orderID)
{
	if (m_TextureAttachmentsColor.size() < (size_t)orderID + 1)
	{
		throw std::runtime_error("Color texture attachment does not exist [orderID = " + std::to_string(orderID) +
			", m_FBO = " + std::to_string(m_FBO) + "]");
	}

	return m_TextureAttachmentsColor.at(orderID);
}

Attachment* Framebuffer::GetAttachmentDepth()
{
	if (m_AttachmentDepth == nullptr)
	{
		throw std::runtime_error("Depth attachment does not exist in current Framebuffer [ " + std::to_string(m_FBO) + " ]");
	}

	return m_AttachmentDepth;
}

Attachment* Framebuffer::GetAttachmentStencil()
{
	if (m_AttachmentStencil == nullptr)
	{
		throw std::runtime_error("Stencil attachment does not exist in current Framebuffer [ " + std::to_string(m_FBO) + " ]");
	}
	return m_AttachmentStencil;
}

Attachment* Framebuffer::GetAttachmentDepthAndStencil()
{
	if (m_AttachmentDepthAndStencil == nullptr)
	{
		throw std::runtime_error("Depth/Stencil attachment does not exist in current Framebuffer [ " + std::to_string(m_FBO) + " ]");
	}

	return m_AttachmentDepthAndStencil;
}

void Framebuffer::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Framebuffer::Resize(uint32_t width, uint32_t height)
{
	if (width  < 0 || width > s_MaxFramebufferSize || height < 0 || height > s_MaxFramebufferSize)
	{
		Log::GetLogger()->warn("Attempted to resize framebuffer to {0}, {1}", width, height);
		return;
	}

	m_Width = width;
	m_Height = height;

	Generate(m_Width, m_Height);
}

Framebuffer::~Framebuffer()
{
	Release();
	m_AttachmentSpecs.clear();
}
