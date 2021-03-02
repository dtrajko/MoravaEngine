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

Attachment::Attachment(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat, unsigned int orderID)
	: m_Width(width), m_Height(height), m_AttachmentType(attachmentType), m_AttachmentFormat(attachmentFormat), m_OrderID(orderID)
{
}

Attachment::~Attachment()
{
}
