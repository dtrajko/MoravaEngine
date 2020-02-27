#pragma once

#include "FramebufferTexture.h"
#include "Renderbuffer.h"

#include <vector>
#include <string>


/**
GL_COLOR_ATTACHMENT0
GL_DEPTH_ATTACHMENT
GL_STENCIL_ATTACHMENT
GL_DEPTH_STENCIL_ATTACHMENT

Each attachment is either a Texture or Renderbuffer Object
*/

class Framebuffer
{

public:
	Framebuffer();
	Framebuffer(unsigned int width, unsigned int height);
	void AddColorAttachment(FramebufferTexture* colorAttachment);
	void AddDepthAttachment(FramebufferTexture* depthAttachment);
	void AddDepthBuffer(Renderbuffer* depthBuffer);
	std::vector<FramebufferTexture*> GetColorAttachments() const { return m_ColorAttachments; };
	FramebufferTexture* GetDepthAttachment() const { return m_DepthAttachment; };
	void Write();
	void Read(unsigned int textureUnit);
	void Unbind();
	~Framebuffer();

private:
	unsigned int fbo;
	unsigned int m_Width;
	unsigned int m_Height;

	std::vector<FramebufferTexture*> m_ColorAttachments;
	FramebufferTexture* m_DepthAttachment;
	Renderbuffer* m_DepthBuffer;
	FramebufferTexture* m_StencilAttachment; // still not in use

};
