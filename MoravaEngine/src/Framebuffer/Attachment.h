#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/HazelTexture.h"


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
	RGB                 = 10, // Hazel::HazelImageFormat
	RGBA32F             = 11, // Hazel::HazelImageFormat
	RG32F               = 12, // Hazel::HazelImageFormat
	SRGB                = 13, // Hazel::HazelImageFormat
	DEPTH32F            = 14, // Hazel::HazelImageFormat
};

class Attachment : public Hazel::HazelTexture
{
public:
	Attachment();
	Attachment(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat, unsigned int orderID, uint32_t framebufferID);
	virtual ~Attachment();

	inline uint32_t GetID() const { return m_ID; };
	inline virtual uint32_t GetWidth() const override { return m_Width; };
	inline virtual uint32_t GetHeight() const override { return m_Height; };

	virtual void Bind(uint32_t slot) const = 0;
	virtual void Unbind() = 0;

	// virtual methods from Hazel::HazelTexture
	virtual Hazel::HazelImageFormat GetFormat() const;
	virtual uint32_t GetMipLevelCount() const { Log::GetLogger()->error("Attachment::GetMipLevelCount - method not implemented!"); return 0; }
	virtual uint64_t GetHash() const { Log::GetLogger()->error("Attachment::GetHash - method not implemented!"); return 0; }
	virtual Hazel::HazelTextureType GetType() const { Log::GetLogger()->error("Attachment::GetType - method not implemented!"); return Hazel::HazelTextureType(); }
	virtual bool operator==(const HazelTexture& other) const { Log::GetLogger()->error("Attachment::operator== - method not implemented!"); return false; }
	virtual void SetData(void* data, uint32_t size) { Log::GetLogger()->error("Attachment::SetData - method not implemented!"); }

protected:
	unsigned int m_ID;
	unsigned int m_Width;
	unsigned int m_Height;
	AttachmentType m_AttachmentType;
	AttachmentFormat m_AttachmentFormat;
	unsigned int m_OrderID; // used for color texture attachments GL_COLOR_ATTACHMENT0 + m_OrderID

	uint32_t m_FramebufferID;

};
