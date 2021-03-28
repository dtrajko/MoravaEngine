#pragma once

#include "Hazel/Core/Ref.h"


enum class AttachmentType
{
	None         = 0,
	Texture      = 1,
	Renderbuffer = 2,
};

enum class AttachmentFormat
{
	None                = 0,
	Color               = 1,
	Depth               = 2, // GL_DEPTH_COMPONENT
	Stencil             = 3, // GL_STENCIL
	Depth_24            = 4, // GL_DEPTH_COMPONENT24
	DepthStencil        = 5,
	Depth_24_Stencil_8  = 6, // GL_DEPTH24_STENCIL8
	RGBA                = 7,
	RGBA16F             = 8,
	RGBA8               = 9,
};


class Attachment : public Hazel::RefCounted
{
public:
	Attachment();
	Attachment(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat, unsigned int orderID, uint32_t framebufferID);
	virtual ~Attachment();

	inline unsigned int GetID() const { return m_ID; };
	inline const unsigned int GetWidth() const { return m_Width; };
	inline const unsigned int GetHeight() const { return m_Height; };

	virtual void Bind(unsigned int slot) = 0;
	virtual void Unbind() = 0;


protected:
	unsigned int m_ID;
	unsigned int m_Width;
	unsigned int m_Height;
	AttachmentType m_AttachmentType;
	AttachmentFormat m_AttachmentFormat;
	unsigned int m_OrderID; // used for color texture attachments GL_COLOR_ATTACHMENT0 + m_OrderID

	uint32_t m_FramebufferID;

};
