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

	void CreateTextureAttachmentColor(unsigned int width, unsigned int height, FBOTextureType txType = FBOTextureType::Color);
	void CreateTextureAttachmentDepth(unsigned int width, unsigned int height, FBOTextureType txType = FBOTextureType::Depth);
	void CreateTextureAttachmentStencil(unsigned int width, unsigned int height, FBOTextureType txType = FBOTextureType::Stencil);

	void CreateBufferAttachmentDepth(unsigned int width, unsigned int height, RBOType formatType = RBOType::Depth);
	void CreateBufferAttachmentStencil(unsigned int width, unsigned int height, RBOType formatType = RBOType::Stencil);
	void CreateBufferAttachmentDepthAndStencil(unsigned int width, unsigned int height, RBOType formatType = RBOType::Depth_24_Stencil_8);

	FramebufferTexture* GetTextureAttachmentColor(unsigned int orderID = 0);

	~Framebuffer();

private:
	unsigned int m_FBO;

	std::vector<FramebufferTexture*> m_TextureAttachmentsColor;
	FramebufferTexture* m_TextureAttachmentDepth;
	FramebufferTexture* m_TextureAttachmentStencil;

	Renderbuffer* m_BufferAttachmentDepth;
	Renderbuffer* m_BufferAttachmentStencil;
	Renderbuffer* m_BufferAttachmentDepthAndStencil;

};
