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
	void AttachTexture(unsigned int width, unsigned int height, FramebufferTextureType type);
	void AttachRenderbuffer(unsigned int width, unsigned int height, RenderbufferFormatType internalFormat);
	~Framebuffer();

private:
	unsigned int m_FBO;

	std::vector<FramebufferTexture*> m_TextureAttachments;
	std::vector<Renderbuffer*> m_RenderbufferAttachments;

	// buffer color
	// buffer depth
	// buffer stencil

	// color attachment (at least one)

};
