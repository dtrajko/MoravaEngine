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
 * Maps between AttachmentFormat and H2M::ImageFormatH2M values
 */
H2M::ImageFormatH2M Attachment::GetFormat() const
{
	switch (m_AttachmentFormat)
	{
	case AttachmentFormat::None:
		return H2M::ImageFormatH2M::None;
		break;
	case AttachmentFormat::Color:
		return H2M::ImageFormatH2M::RGB;
		break;
	case AttachmentFormat::Depth:
		return H2M::ImageFormatH2M::Depth;
		break;
	case AttachmentFormat::Stencil:
		return H2M::ImageFormatH2M::DEPTH24STENCIL8;
		break;
	case AttachmentFormat::Depth_24:
		return H2M::ImageFormatH2M::Depth;
		break;
	case AttachmentFormat::DepthStencil:
		return H2M::ImageFormatH2M::DEPTH24STENCIL8;
		break;
	case AttachmentFormat::Depth_24_Stencil_8:
		return H2M::ImageFormatH2M::DEPTH24STENCIL8;
		break;
	case AttachmentFormat::RGBA:
		return H2M::ImageFormatH2M::RGBA;
		break;
	case AttachmentFormat::RGBA16F:
		return H2M::ImageFormatH2M::RGBA16F;
		break;
	case AttachmentFormat::RGBA8:
		return H2M::ImageFormatH2M::RGBA;
		break;
	case AttachmentFormat::RED_INTEGER:
		return H2M::ImageFormatH2M::RED_INTEGER;
		break;
	default:
		return H2M::ImageFormatH2M::None;
		break;
	}
}

std::pair<uint32_t, uint32_t> Attachment::GetMipSize(uint32_t mip) const
{
	Log::GetLogger()->error("Attachment::GetMipSize({0}) - method not implemented!", mip);
	return std::pair<uint32_t, uint32_t>();
}
