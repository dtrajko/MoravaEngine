#pragma once

#include "FramebufferTexture.h"
#include "Renderbuffer.h"

#include <vector>


class Framebuffer
{
public:
	Framebuffer();
	void Bind();
	void Unbind();
	bool CheckStatus();

	void CreateTextureAttachmentColor(unsigned int width, unsigned int height, FramebufferTextureType txType);
	void CreateTextureAttachmentDepth(unsigned int width, unsigned int height, FramebufferTextureType txType);
	void CreateTextureAttachmentStencil(unsigned int width, unsigned int height, FramebufferTextureType txType);

	void CreateBufferAttachmentDepth(unsigned int width, unsigned int height, RenderbufferFormatType formatType);
	void CreateBufferAttachmentStencil(unsigned int width, unsigned int height, RenderbufferFormatType formatType);

	~Framebuffer();

private:
	unsigned int m_FBO;

	std::vector<FramebufferTexture*> m_TextureAttachmentsColor;
	FramebufferTexture* m_TextureAttachmentDepth;
	FramebufferTexture* m_TextureAttachmentStencil;

	Renderbuffer* m_BufferAttachmentDepth;
	Renderbuffer* m_BufferAttachmentStencil;

	// buffer color
	// buffer depth
	// buffer stencil

	// color attachment (at least one)

};
