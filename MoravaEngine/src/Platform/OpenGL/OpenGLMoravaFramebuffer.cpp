#include "OpenGLMoravaFramebuffer.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include <GL/glew.h>

#include <stdexcept>


static const uint32_t s_MaxFramebufferSize = 8192;

OpenGLMoravaFramebuffer::OpenGLMoravaFramebuffer()
{
	m_FBO = 0;

	m_FramebufferSpecs = {};
	m_FramebufferSpecs.Width = Application::Get()->GetWindow()->GetWidth();
	m_FramebufferSpecs.Height = Application::Get()->GetWindow()->GetHeight();
	m_FramebufferSpecs.ClearColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_FramebufferSpecs.attachmentType = AttachmentType::None;
	m_FramebufferSpecs.attachmentFormat = AttachmentFormat::None;
	m_FramebufferSpecs.Samples = 1;
	m_FramebufferSpecs.SwapChainTarget = false;

	m_TextureAttachmentsColor = std::vector<FramebufferTexture*>();
	m_AttachmentDepth = nullptr;
	m_AttachmentStencil = nullptr;
	m_AttachmentDepthAndStencil = nullptr;

	Resize(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height);
}

OpenGLMoravaFramebuffer::OpenGLMoravaFramebuffer(unsigned int width, unsigned int height)
	: OpenGLMoravaFramebuffer()
{
	m_FramebufferSpecs.Width = width;
	m_FramebufferSpecs.Height = height;
}

OpenGLMoravaFramebuffer::OpenGLMoravaFramebuffer(FramebufferSpecification spec)
{
	m_FramebufferSpecs = spec;
	OpenGLMoravaFramebuffer(spec.Width, spec.Height);
}

void OpenGLMoravaFramebuffer::AddColorAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	FramebufferSpecification fbSpecs;
	fbSpecs.Width = width;
	fbSpecs.Height = height;
	fbSpecs.attachmentType = attachmentType;
	fbSpecs.attachmentFormat = attachmentFormat;
	fbSpecs.ClearColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_ColorAttachmentSpecs.push_back(fbSpecs);
}

void OpenGLMoravaFramebuffer::AddDepthAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (m_RenderbufferAttachmentSpec.size()) {
		Log::GetLogger()->error("Depth attachment specification already exists!");
	}

	FramebufferSpecification fbSpecs;
	fbSpecs.Width = width;
	fbSpecs.Height = height;
	fbSpecs.attachmentType = attachmentType;
	fbSpecs.attachmentFormat = attachmentFormat;
	fbSpecs.ClearColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_RenderbufferAttachmentSpec.push_back(fbSpecs);
}


void OpenGLMoravaFramebuffer::AddColorAttachment(FramebufferSpecification specs)
{
	m_ColorAttachmentSpecs.push_back(specs);
}

void OpenGLMoravaFramebuffer::AddDepthAttachment(FramebufferSpecification specs)
{
	if (m_RenderbufferAttachmentSpec.size()) {
		Log::GetLogger()->error("Depth attachment specification already exists!");
	}

	m_RenderbufferAttachmentSpec.push_back(specs);
}

void OpenGLMoravaFramebuffer::Generate(unsigned int width, unsigned int height)
{
	if (m_FBO) Release();

	m_FramebufferSpecs.Width = width;
	m_FramebufferSpecs.Height = height;

	m_Multisample = m_FramebufferSpecs.Samples > 1;

	glGenFramebuffers(1, &m_FBO);
	Bind(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height);

	// color attachments
	std::vector<GLenum> colorAttachments;
	int colorAttachmentIndex = 0;
	for (FramebufferSpecification attachmentSpecs : m_ColorAttachmentSpecs)
	{
		// m_FramebufferSpecs.Width = attachmentSpecs.Width;
		// m_FramebufferSpecs.Height = attachmentSpecs.Height;

		switch (attachmentSpecs.attachmentFormat)
		{
		case AttachmentFormat::Color:
			CreateTextureAttachmentColor(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height, m_Multisample, attachmentSpecs.attachmentFormat);
			Log::GetLogger()->debug("OpenGLMoravaFramebuffer::Generate [AttachmentFormat::Color, Multisample: {0}, {1}x{2}]",
				m_Multisample, width, height);
			break;
		case AttachmentFormat::RGBA:
			CreateTextureAttachmentColor(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height, m_Multisample,
				attachmentSpecs.attachmentFormat);
			Log::GetLogger()->debug("OpenGLMoravaFramebuffer::Generate [AttachmentFormat::RGBA, Multisample: {0}, {1}x{2}]",
				m_Multisample, width, height);
			break;
		case AttachmentFormat::RGBA16F:
			CreateTextureAttachmentColor(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height, m_Multisample,
				attachmentSpecs.attachmentFormat);
			Log::GetLogger()->debug("OpenGLMoravaFramebuffer::Generate [AttachmentFormat::RGBA16F, Multisample: {0}, {1}x{2}]",
				m_Multisample, width, height);
			break;
		case AttachmentFormat::RGBA8:
			CreateTextureAttachmentColor(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height, m_Multisample,
				attachmentSpecs.attachmentFormat);
			Log::GetLogger()->debug("OpenGLMoravaFramebuffer::Generate [AttachmentFormat::RGBA8, Multisample: {0}, {1}x{2}]",
				m_Multisample, width, height);
			break;
		default:
			Log::GetLogger()->error("Color attachment format '{0}' not supported.", attachmentSpecs.attachmentFormat);
			break;
		}

		// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
		colorAttachments.push_back(GL_COLOR_ATTACHMENT0 + colorAttachmentIndex);
		colorAttachmentIndex++;
	}

	glDrawBuffers((GLsizei)colorAttachments.size(), colorAttachments.data());

	// renderbuffer attachments
	if (m_RenderbufferAttachmentSpec.size()) {
		auto attachmentSpecs = m_RenderbufferAttachmentSpec[0];
		switch (attachmentSpecs.attachmentFormat)
		{
		case AttachmentFormat::Depth:
			CreateAttachmentDepth(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height, m_Multisample,
				attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			Log::GetLogger()->debug("OpenGLMoravaFramebuffer::Generate [AttachmentFormat::Depth, Multisample: {0}, {1}x{2}]",
				m_Multisample, width, height);
			break;
		case AttachmentFormat::DepthStencil:
			CreateAttachmentDepthAndStencil(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height, m_Multisample,
				attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			Log::GetLogger()->debug("OpenGLMoravaFramebuffer::Generate [AttachmentFormat::DepthStencil, Multisample: {0}, {1}x{2}]",
				m_Multisample, width, height);
			break;
		case AttachmentFormat::Depth_24:
			CreateAttachmentDepth(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height, m_Multisample,
				attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			Log::GetLogger()->debug("OpenGLMoravaFramebuffer::Generate [AttachmentFormat::Depth_24, Multisample: {0}, {1}x{2}]",
				m_Multisample, width, height);
			break;
		case AttachmentFormat::Depth_24_Stencil_8:
			CreateAttachmentDepthAndStencil(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height, m_Multisample,
				attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			Log::GetLogger()->debug("OpenGLMoravaFramebuffer::Generate [AttachmentFormat::Depth_24_Stencil_8, Multisample: {0}, {1}x{2}]",
				m_Multisample, width, height);
			break;
		case AttachmentFormat::Stencil:
			CreateAttachmentStencil(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height, m_Multisample,
				attachmentSpecs.attachmentType, attachmentSpecs.attachmentFormat);
			Log::GetLogger()->debug("OpenGLMoravaFramebuffer::Generate [AttachmentFormat::Stencil, Multisample: {0}, {1}x{2}]",
				m_Multisample, width, height);
			break;
		default:
			Log::GetLogger()->error("Depth attachment format '{0}' not supported.", attachmentSpecs.attachmentFormat);
			break;
		}
	}
}

void OpenGLMoravaFramebuffer::Release()
{
	// Log::GetLogger()->info("OpenGLMoravaFramebuffer::Release");

	for (auto& textureAttachment : m_TextureAttachmentsColor)
		delete textureAttachment;

	m_TextureAttachmentsColor.clear();

	glDeleteFramebuffers(1, &m_FBO);

	m_FBO = 0;
}

void OpenGLMoravaFramebuffer::CreateTextureAttachmentColor(unsigned int width, unsigned int height, bool isMultisample,
	AttachmentFormat attachmentFormat)
{
	FramebufferTexture* texture = new FramebufferTexture(width, height, isMultisample,
		attachmentFormat, (unsigned int)m_TextureAttachmentsColor.size());
	m_TextureAttachmentsColor.push_back(texture);

	Log::GetLogger()->info("Framebuffer::CreateTextureAttachmentColor [ID={0}, Multisample: {1}, {2}x{3}]",
		texture->GetID(), isMultisample, texture->GetWidth(), texture->GetHeight());
}

void OpenGLMoravaFramebuffer::CreateAttachmentDepth(unsigned int width, unsigned int height, bool isMultisample,
	AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (attachmentType == AttachmentType::Texture)
		m_AttachmentDepth = Hazel::Ref<FramebufferTexture>::Create(width, height, isMultisample, attachmentFormat, 0);
	else if (attachmentType == AttachmentType::Renderbuffer)
		m_AttachmentDepth = Hazel::Ref<Renderbuffer>::Create(width, height, attachmentFormat, 0, m_FBO);
}

void OpenGLMoravaFramebuffer::CreateAttachmentStencil(unsigned int width, unsigned int height, bool isMultisample,
	AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (attachmentType == AttachmentType::Texture)
		m_AttachmentStencil = Hazel::Ref<FramebufferTexture>::Create(width, height, isMultisample, attachmentFormat, 0);
	else if (attachmentType == AttachmentType::Renderbuffer)
		m_AttachmentStencil = Hazel::Ref<Renderbuffer>::Create(width, height, attachmentFormat, 0, m_FBO);
}

void OpenGLMoravaFramebuffer::CreateAttachmentDepthAndStencil(unsigned int width, unsigned int height, bool isMultisample,
	AttachmentType attachmentType, AttachmentFormat attachmentFormat)
{
	if (attachmentType == AttachmentType::Texture)
		m_AttachmentDepthAndStencil = Hazel::Ref<FramebufferTexture>::Create(width, height, isMultisample, attachmentFormat, 0);
	else if (attachmentType == AttachmentType::Renderbuffer)
		m_AttachmentDepthAndStencil = Hazel::Ref<Renderbuffer>::Create(width, height, attachmentFormat, 0, m_FBO);
}

void OpenGLMoravaFramebuffer::Bind() const
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glViewport(0, 0, m_FramebufferSpecs.Width, m_FramebufferSpecs.Height);
}

void OpenGLMoravaFramebuffer::Unbind() const
{
	// unbind custom framebuffer and make the default framebuffer active
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_FramebufferSpecs.Width, m_FramebufferSpecs.Height);
}

void OpenGLMoravaFramebuffer::Bind(unsigned int width, unsigned int height)
{
	m_FramebufferSpecs.Width = width;
	m_FramebufferSpecs.Height = height;

	Bind();
}

void OpenGLMoravaFramebuffer::Unbind(unsigned int width, unsigned int height)
{
	m_FramebufferSpecs.Width = width;
	m_FramebufferSpecs.Height = height;

	Unbind();
}

bool OpenGLMoravaFramebuffer::CheckStatus()
{
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

FramebufferTexture* OpenGLMoravaFramebuffer::GetTextureAttachmentColor(unsigned int orderID)
{
	if (m_TextureAttachmentsColor.size() < (size_t)orderID + 1)
	{
		throw std::runtime_error("Color texture attachment does not exist [orderID = " + std::to_string(orderID) +
			", m_FBO = " + std::to_string(m_FBO) + "]");
	}

	return m_TextureAttachmentsColor.at(orderID);
}

Hazel::Ref<Attachment> OpenGLMoravaFramebuffer::GetAttachmentDepth()
{
	if (!m_AttachmentDepth)
	{
		throw std::runtime_error("Depth attachment does not exist in current Framebuffer [ " + std::to_string(m_FBO) + " ]");
	}

	return m_AttachmentDepth;
}

Hazel::Ref<Attachment> OpenGLMoravaFramebuffer::GetAttachmentStencil()
{
	if (!m_AttachmentStencil)
	{
		throw std::runtime_error("Stencil attachment does not exist in current Framebuffer [ " + std::to_string(m_FBO) + " ]");
	}
	return m_AttachmentStencil;
}

Hazel::Ref<Attachment> OpenGLMoravaFramebuffer::GetAttachmentDepthAndStencil()
{
	if (!m_AttachmentDepthAndStencil)
	{
		throw std::runtime_error("Depth/Stencil attachment does not exist in current Framebuffer [ " + std::to_string(m_FBO) + " ]");
	}

	return m_AttachmentDepthAndStencil;
}

Hazel::Ref<MoravaFramebuffer> OpenGLMoravaFramebuffer::Create(const FramebufferSpecification& spec)
{
	return MoravaFramebuffer::Create(spec);
}

void OpenGLMoravaFramebuffer::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void OpenGLMoravaFramebuffer::Resize(uint32_t width, uint32_t height)
{
	if (width  < 0 || width > s_MaxFramebufferSize || height < 0 || height > s_MaxFramebufferSize)
	{
		Log::GetLogger()->warn("Attempted to resize framebuffer to {0}, {1}", width, height);
		return;
	}

	m_FramebufferSpecs.Width = width;
	m_FramebufferSpecs.Height = height;

	Generate(m_FramebufferSpecs.Width, m_FramebufferSpecs.Height);
}

void OpenGLMoravaFramebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
{
	Resize(width, height);
}

void OpenGLMoravaFramebuffer::BindTexture(uint32_t attachmentIndex, uint32_t slot) const
{
	if (attachmentIndex >= m_TextureAttachmentsColor.size()) return;

	m_TextureAttachmentsColor.at(attachmentIndex)->Bind(slot);
}

Hazel::RendererID OpenGLMoravaFramebuffer::GetRendererID() const
{
	return m_FBO;
}

Hazel::Ref<Hazel::HazelImage2D> OpenGLMoravaFramebuffer::GetImage(uint32_t attachmentIndex) const
{
	return Hazel::Ref<Hazel::HazelImage2D>();
}

Hazel::Ref<Hazel::HazelImage2D> OpenGLMoravaFramebuffer::GetDepthImage() const
{
	return Hazel::Ref<Hazel::HazelImage2D>();
}

//	Hazel::RendererID OpenGLMoravaFramebuffer::GetColorAttachmentRendererID() const
//	{
//		return Hazel::RendererID();
//	}

//	Hazel::RendererID OpenGLMoravaFramebuffer::GetDepthAttachmentRendererID() const
//	{
//		return Hazel::RendererID();
//	}

const Hazel::HazelFramebufferSpecification& OpenGLMoravaFramebuffer::GetSpecification() const
{
	return m_HazelFramebufferSpecs;
}

OpenGLMoravaFramebuffer::~OpenGLMoravaFramebuffer()
{
	Release();
	m_ColorAttachmentSpecs.clear();
	m_RenderbufferAttachmentSpec.clear();
}
