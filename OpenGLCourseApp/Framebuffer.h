#pragma once

#include "FramebufferTexture.h"
#include "Renderbuffer.h"

#include <vector>
#include <string>
#include <map>


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
	inline FramebufferTexture* GetColorAttachment() const { return m_ColorAttachment; };
	inline FramebufferTexture* GetDepthAttachment() const { return m_DepthAttachment; };
	inline Renderbuffer* GetDepthBuffer() const { return m_DepthBuffer; };
	inline unsigned int GetID() const { return fbo; };
	void Bind();
	void Unbind();
	~Framebuffer();

private:
	unsigned int fbo;
	unsigned int m_Width;
	unsigned int m_Height;

	FramebufferTexture* m_ColorAttachment = nullptr;
	FramebufferTexture* m_DepthAttachment = nullptr;
	Renderbuffer* m_DepthBuffer;
	FramebufferTexture* m_StencilAttachment; // still not in use

};
