#pragma once

#include "FramebufferTexture.h"
#include "Renderbuffer.h"

#include <vector>


class Framebuffer
{
public:
	Framebuffer();
	Framebuffer(unsigned int width, unsigned int height);
	void Bind(unsigned int width, unsigned int height);
	void Unbind(unsigned int width, unsigned int height);
	bool CheckStatus();

	void CreateTextureAttachmentColor(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat = AttachmentFormat::Color);
	void CreateAttachmentDepth(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth);
	void CreateAttachmentStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Stencil);
	void CreateAttachmentDepthAndStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth_24_Stencil_8);

	inline FramebufferTexture* GetTextureAttachmentColor(unsigned int orderID = 0) { return m_TextureAttachmentsColor.at(orderID); }
	inline Attachment* GetAttachmentDepth() { return m_AttachmentDepth; };
	inline Attachment* GetAttachmentStencil() { return m_AttachmentStencil; };
	inline Attachment* GetAttachmentDepthAndStencil() { return m_AttachmentDepthAndStencil; };

	void Clear();

	~Framebuffer();

private:
	unsigned int m_FBO;

	std::vector<FramebufferTexture*> m_TextureAttachmentsColor;
	Attachment* m_AttachmentDepth;
	Attachment* m_AttachmentStencil;
	Attachment* m_AttachmentDepthAndStencil;

};
