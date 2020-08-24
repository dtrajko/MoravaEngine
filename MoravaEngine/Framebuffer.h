#pragma once

#include "FramebufferTexture.h"
#include "Renderbuffer.h"

#include <vector>


struct FramebufferSpecification
{
	uint32_t Width, Height;
	// FramebufferFormat Format = 
	uint32_t Samples = 1;

	bool SwapChainTarget = false;
};

class Framebuffer
{
public:
	Framebuffer();
	Framebuffer(unsigned int width, unsigned int height);
	~Framebuffer();

	void Bind(unsigned int width, unsigned int height);
	void Unbind(unsigned int width, unsigned int height);
	bool CheckStatus();

	void CreateTextureAttachmentColor(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat = AttachmentFormat::Color);
	void CreateAttachmentDepth(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth);
	void CreateAttachmentStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Stencil);
	void CreateAttachmentDepthAndStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth_24_Stencil_8);

	FramebufferTexture* GetTextureAttachmentColor(unsigned int orderID = 0);
	Attachment* GetAttachmentDepth();
	Attachment* GetAttachmentStencil();
	Attachment* GetAttachmentDepthAndStencil();

	void Clear();

	// TODO
	void Invalidate();
	void Resize(uint32_t width, uint32_t height);

private:
	uint32_t m_Width;
	uint32_t m_Height;

	unsigned int m_FBO;

	std::vector<FramebufferTexture*> m_TextureAttachmentsColor;
	Attachment* m_AttachmentDepth;
	Attachment* m_AttachmentStencil;
	Attachment* m_AttachmentDepthAndStencil;

};
