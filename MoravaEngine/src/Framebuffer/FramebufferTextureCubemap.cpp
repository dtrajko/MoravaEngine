#include "Framebuffer/FramebufferTextureCubemap.h"

#include "../../Log.h"

#include <GL/glew.h>

#include <stdexcept>


FramebufferTextureCubemap::FramebufferTextureCubemap()
	: FramebufferTexture()
{
	m_Level = 0;
	InitSpecification();
}

FramebufferTextureCubemap::FramebufferTextureCubemap(unsigned int width, unsigned int height, bool isMultisample,
	AttachmentFormat attachmentFormat, unsigned int orderID)
	: FramebufferTexture(width, height, isMultisample, AttachmentType::Texture, attachmentFormat, orderID)
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
	case AttachmentFormat::RGBA16F:
		m_Attachment = GL_COLOR_ATTACHMENT0 + orderID;
		m_Spec.InternalFormat = GL_RGBA;
		m_Spec.Format = GL_RGBA;
		m_Spec.Type = GL_RGBA16F;
		break;
	case AttachmentFormat::RGBA8:
		m_Attachment = GL_COLOR_ATTACHMENT0 + orderID;
		m_Spec.InternalFormat = GL_RGBA;
		m_Spec.Format = GL_RGBA;
		m_Spec.Type = GL_RGBA8;
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

FramebufferTextureCubemap::FramebufferTextureCubemap(Texture::Specification spec, unsigned int orderID)
	: FramebufferTextureCubemap()
{
	m_Attachment = GL_COLOR_ATTACHMENT0 + orderID;

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
	m_Spec.Samples = spec.Samples;
	m_Spec.IsMultisample = spec.IsMultisample;

	OpenGLCreate();
}

void FramebufferTextureCubemap::InitSpecification()
{
	m_Spec.Border = 0;
	m_Spec.Texture_Wrap_S = GL_REPEAT; // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	m_Spec.Texture_Wrap_T = GL_REPEAT; // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	m_Spec.Texture_Wrap_R = GL_REPEAT; // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	m_Spec.Texture_Min_Filter = GL_LINEAR;
	m_Spec.Texture_Mag_Filter = GL_LINEAR;
}

void FramebufferTextureCubemap::OpenGLCreate()
{
	glGenTextures(1, &m_ID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);

	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_Level, m_Spec.InternalFormat, m_Width, m_Height, m_Spec.Border, m_Spec.Format, m_Spec.Type, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, m_Spec.Texture_Wrap_S); // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, m_Spec.Texture_Wrap_T); // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, m_Spec.Texture_Wrap_R); // GL_CLAMP_TO_EDGE is causing problems with reflection FBO
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_Spec.Texture_Min_Filter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_Spec.Texture_Mag_Filter);

	for (unsigned int i = 0; i < 6; ++i)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, m_Attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_ID, 0);
	}
}

void FramebufferTextureCubemap::Bind(unsigned int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);
}

void FramebufferTextureCubemap::Unbind()
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

FramebufferTextureCubemap::~FramebufferTextureCubemap()
{
	glDeleteTextures(1, &m_ID);
}
