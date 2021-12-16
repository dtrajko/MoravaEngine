#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"

#include "Framebuffer/Attachment.h"
#include "Framebuffer/FramebufferTexture.h"


struct FramebufferSpecification
{
	uint32_t Width = 1280;
	uint32_t Height = 720;
	glm::vec4 ClearColor;

	AttachmentType attachmentType;
	AttachmentFormat attachmentFormat;

	uint32_t Samples = 1; // multisampling

	// SwapChainTarget = screen buffer (i.e. no framebuffer)
	bool SwapChainTarget = false;
};


class MoravaFramebuffer : public H2M::FramebufferH2M
{
public:
	static H2M::RefH2M<MoravaFramebuffer> Create(uint32_t width, uint32_t height);
	static H2M::RefH2M<MoravaFramebuffer> Create(FramebufferSpecification spec);

	// virtual methods from H2M::FramebufferH2M
	virtual void Bind() const = 0;
	virtual void Unbind() const = 0;
	virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) = 0;
	virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;
	virtual void AddResizeCallback(const std::function<void(H2M::RefH2M<H2M::FramebufferH2M>)>& func) = 0;
	virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const = 0;
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual uint32_t GetRendererID() const = 0;
	virtual H2M::RefH2M<H2M::Image2D_H2M> GetImage(uint32_t attachmentIndex = 0) const = 0;
	virtual H2M::RefH2M<H2M::Image2D_H2M> GetDepthImage() const = 0;
	virtual const H2M::FramebufferSpecificationH2M& GetSpecification() const = 0;

	// virtual methods MoravaFramebufer
	virtual void Generate(unsigned int width, unsigned int height) = 0; // Invalidate() in Hazel
	virtual void AddColorAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat) = 0;
	virtual void AddDepthAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat) = 0;
	virtual void AddColorAttachment(FramebufferSpecification specs) = 0; // the generic one based on FramebufferSpecification
	virtual void AddDepthAttachment(FramebufferSpecification specs) = 0;
	virtual FramebufferTexture* GetTextureAttachmentColor(unsigned int orderID = 0) = 0;
	virtual H2M::RefH2M<Attachment> GetAttachmentDepth() = 0;
	virtual H2M::RefH2M<Attachment> GetAttachmentStencil() = 0;
	virtual H2M::RefH2M<Attachment> GetAttachmentDepthAndStencil() = 0;
	virtual void Bind(unsigned int width, unsigned int height) = 0;
	virtual void Unbind(unsigned int width, unsigned int height) = 0;
	virtual bool CheckStatus() = 0;
	virtual void Clear() = 0;

};
