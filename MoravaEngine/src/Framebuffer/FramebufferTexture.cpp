#include "FramebufferTexture.h"

#include "Core/Log.h"

#include <GL/glew.h>

#include <stdexcept>


FramebufferTexture::FramebufferTexture()
	: Attachment()
{
	m_Level = 0;
	InitSpecification();
}

FramebufferTexture::FramebufferTexture(MoravaTexture::Specification spec, unsigned int orderID)
	: FramebufferTexture()
{
	m_Spec.InternalFormat = spec.InternalFormat;
	m_Spec.Width = spec.Width;
	m_Spec.Height = spec.Height;
	m_Spec.Border = spec.Border;
	m_Spec.Format = spec.Format;
	m_Spec.Type = spec.Type;
	m_Spec.Texture_Wrap_S = spec.Texture_Wrap_S;
	m_Spec.Texture_Wrap_T = spec.Texture_Wrap_T;
	m_Spec.Texture_Min_Filter = spec.Texture_Min_Filter;
	m_Spec.Texture_Mag_Filter = spec.Texture_Mag_Filter;
	m_Spec.MipLevel = spec.MipLevel;
	m_Spec.FlipVertically = spec.FlipVertically;
	m_Spec.BitDepth = spec.BitDepth;
	m_Spec.IsSampler = spec.IsSampler;
	m_Spec.IsMultisample = spec.IsMultisample;

	OpenGLCreate();
}

FramebufferTexture::FramebufferTexture(unsigned int width, unsigned int height, bool isMultisample,
	AttachmentFormat attachmentFormat, unsigned int orderID)
	: Attachment(width, height, AttachmentType::Texture, attachmentFormat, orderID, 0)
{
	m_Level = 0;
	InitSpecification();

	m_Spec.IsMultisample = isMultisample;

	switch (attachmentFormat)
	{
	case AttachmentFormat::Color:
		m_Attachment = GL_COLOR_ATTACHMENT0 + orderID;
		m_Spec.InternalFormat = GL_RGB;
		m_Spec.Format = GL_RGB;
		m_Spec.Type = GL_UNSIGNED_BYTE;
		break;
	case AttachmentFormat::RGBA:
		m_Attachment = GL_COLOR_ATTACHMENT0 + orderID;
		m_Spec.InternalFormat = GL_RGBA;
		m_Spec.Format = GL_RGBA;
		m_Spec.Type = GL_UNSIGNED_BYTE;
		break;
	case AttachmentFormat::RGBA8:
		m_Attachment = GL_COLOR_ATTACHMENT0 + orderID;
		m_Spec.InternalFormat = GL_RGBA;
		m_Spec.Format = GL_RGBA;
		m_Spec.Type = GL_RGBA8;
		break;
	case AttachmentFormat::RGBA16F:
		m_Attachment = GL_COLOR_ATTACHMENT0 + orderID;
		m_Spec.InternalFormat = GL_RGBA;
		m_Spec.Format = GL_RGBA;
		m_Spec.Type = GL_RGBA16F;
		break;
	case AttachmentFormat::RED_INTEGER:
		m_Attachment = GL_COLOR_ATTACHMENT0 + orderID;
		m_Spec.InternalFormat = GL_R16I/*GL_R32I*/,
		m_Spec.Format = GL_RED_INTEGER;
		m_Spec.Type = GL_UNSIGNED_BYTE;
		break;
	case AttachmentFormat::Depth:
		m_Attachment = GL_DEPTH_ATTACHMENT;
		m_Spec.InternalFormat = GL_DEPTH_COMPONENT32;
		m_Spec.Format = GL_DEPTH_COMPONENT;
		m_Spec.Type = GL_FLOAT;
		break;
	case AttachmentFormat::Stencil:
		m_Attachment = GL_STENCIL_ATTACHMENT;
		m_Spec.InternalFormat = GL_STENCIL_INDEX;
		m_Spec.Format = GL_STENCIL_INDEX;
		m_Spec.Type = GL_FLOAT;
		break;
	case AttachmentFormat::DepthStencil:
	case AttachmentFormat::Depth_24_Stencil_8:
		m_Attachment = GL_DEPTH_STENCIL_ATTACHMENT;
		m_Spec.InternalFormat = GL_DEPTH24_STENCIL8;
		m_Spec.Format = GL_DEPTH_STENCIL;
		m_Spec.Type = GL_UNSIGNED_INT_24_8;
		break;
	default:
		throw std::runtime_error("AttachmentFormat not supported!");
		return;
	}

	OpenGLCreate();
}

FramebufferTexture::FramebufferTexture(unsigned int width, unsigned int height, bool isMultisample,
	AttachmentType attachmentType, AttachmentFormat attachmentFormat, unsigned int orderID)
	: Attachment(width, height, attachmentType, attachmentFormat, orderID, 0)
{
	m_Level = 0;
	InitSpecification();

	m_Spec.IsMultisample = isMultisample;

	OpenGLCreate();
}

void FramebufferTexture::InitSpecification()
{
	m_Spec.Border = 0;

	m_Spec.Texture_Min_Filter = GL_LINEAR;
	m_Spec.Texture_Mag_Filter = GL_LINEAR;

	m_Spec.Texture_Wrap_S = GL_REPEAT; // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	m_Spec.Texture_Wrap_T = GL_REPEAT; // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
}

void FramebufferTexture::OpenGLCreate()
{
	if (m_Spec.IsMultisample)
	{
		glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_ID);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ID);

		// TODO: Create Hazel texture object based on format here
		if (m_Spec.Format == (unsigned int)AttachmentFormat::RGBA16F)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Spec.Samples, GL_RGBA16F, m_Spec.Width, m_Spec.Height, GL_FALSE);
		}
		else if (m_Spec.Format == (unsigned int)AttachmentFormat::RGBA8)
		{
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Spec.Samples, GL_RGBA8, m_Spec.Width, m_Spec.Height, GL_FALSE);
		}
		// glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		// glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_ID, 0);
	}
	else
	{
		glGenTextures(1, &m_ID);
		glBindTexture(GL_TEXTURE_2D, m_ID);

		glTexImage2D(GL_TEXTURE_2D, m_Level, m_Spec.InternalFormat, m_Width, m_Height, m_Spec.Border, m_Spec.Format, m_Spec.Type, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_Spec.Texture_Min_Filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_Spec.Texture_Mag_Filter);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_Spec.Texture_Wrap_S); // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_Spec.Texture_Wrap_T); // GL_CLAMP_TO_EDGE is causing problems with reflection FBO

		glFramebufferTexture(GL_FRAMEBUFFER, m_Attachment, m_ID, 0);
	}
}

void FramebufferTexture::Bind(unsigned int slot) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_ID);
}

void FramebufferTexture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

GLenum FramebufferTexture::TextureTarget(bool multisampled)
{
	return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
}

FramebufferTexture::~FramebufferTexture()
{
	glDeleteTextures(1, &m_ID);
}
