#include "Framebuffer/Attachment.h"


Attachment::Attachment()
{
	m_ID = 0;
	m_Width = 0;
	m_Height = 0;
	m_AttachmentType = AttachmentType::None;
	m_AttachmentFormat = AttachmentFormat::None;
	m_OrderID = 0;
}

Attachment::Attachment(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat, unsigned int orderID, uint32_t framebufferID)
	: m_Width(width), m_Height(height), m_AttachmentType(attachmentType), m_AttachmentFormat(attachmentFormat), m_OrderID(orderID), m_FramebufferID(framebufferID)
{
}

Attachment::~Attachment()
{
}

/****
 * Maps between AttachmentFormat and Hazel::HazelImageFormat values
 */
Hazel::HazelImageFormat Attachment::GetFormat() const
{
	switch (m_AttachmentFormat)
	{
	case AttachmentFormat::None:
		return Hazel::HazelImageFormat::None;
		break;
	case AttachmentFormat::Color:
		return Hazel::HazelImageFormat::RGB;
		break;
	case AttachmentFormat::Depth:
		return Hazel::HazelImageFormat::Depth;
		break;
	case AttachmentFormat::Stencil:
		return Hazel::HazelImageFormat::DEPTH24STENCIL8;
		break;
	case AttachmentFormat::Depth_24:
		return Hazel::HazelImageFormat::Depth;
		break;
	case AttachmentFormat::DepthStencil:
		return Hazel::HazelImageFormat::DEPTH24STENCIL8;
		break;
	case AttachmentFormat::Depth_24_Stencil_8:
		return Hazel::HazelImageFormat::DEPTH24STENCIL8;
		break;
	case AttachmentFormat::RGBA:
		return Hazel::HazelImageFormat::RGBA;
		break;
	case AttachmentFormat::RGBA16F:
		return Hazel::HazelImageFormat::RGBA16F;
		break;
	case AttachmentFormat::RGBA8:
		return Hazel::HazelImageFormat::RGBA;
		break;
	default:
		return Hazel::HazelImageFormat::None;
		break;
	}
}
