#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/TextureH2M.h"


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
	RGB                 = 10, // H2M::ImageFormatH2M
	RGBA32F             = 11, // H2M::ImageFormatH2M
	RG32F               = 12, // H2M::ImageFormatH2M
	SRGB                = 13, // H2M::ImageFormatH2M
	DEPTH32F            = 14, // H2M::ImageFormatH2M
	RED_INTEGER         = 15, // For storing EntityIDs
};

class Attachment : public H2M::TextureH2M
{
public:
	Attachment();
	Attachment(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat, unsigned int orderID, uint32_t framebufferID);
	virtual ~Attachment();

	inline uint32_t GetID() const { return m_ID; };
	inline uint32_t GetRendererID() const { return m_ID; };
	inline virtual uint32_t GetWidth() const override { return m_Width; };
	inline virtual uint32_t GetHeight() const override { return m_Height; };

	virtual void Bind(uint32_t slot) const = 0;
	virtual void Unbind() = 0;

	// virtual methods from H2M::HazelTexture
	virtual H2M::ImageFormatH2M GetFormat() const override;
	virtual uint64_t GetHash() const override { Log::GetLogger()->error("Attachment::GetHash - method not implemented!"); return 0; }
	virtual H2M::TextureTypeH2M GetType() const override { Log::GetLogger()->error("Attachment::GetType - method not implemented!"); return H2M::TextureTypeH2M(); }
	virtual bool operator==(const TextureH2M& other) const override { Log::GetLogger()->error("Attachment::operator== - method not implemented!"); return false; }
	virtual void SetData(void* data, uint32_t size) override { Log::GetLogger()->error("Attachment::SetData - method not implemented!"); }
	virtual uint32_t GetMipLevelCount() const override { Log::GetLogger()->error("Attachment::GetMipLevelCount - method not implemented!"); return 0; }
	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

protected:
	unsigned int m_ID;
	unsigned int m_Width;
	unsigned int m_Height;
	AttachmentType m_AttachmentType;
	AttachmentFormat m_AttachmentFormat;
	unsigned int m_OrderID; // used for color texture attachments GL_COLOR_ATTACHMENT0 + m_OrderID

	uint32_t m_FramebufferID;

};
