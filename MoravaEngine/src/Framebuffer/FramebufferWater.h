#pragma once

#include "Framebuffer/FramebufferTexture.h"
#include "Framebuffer/Renderbuffer.h"

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

class FramebufferWater
{

public:
	FramebufferWater();
	FramebufferWater(unsigned int width, unsigned int height);
	void AddColorAttachment(FramebufferTexture* colorAttachment);
	void AddDepthAttachment(FramebufferTexture* depthAttachment);
	void AddDepthBuffer(Renderbuffer* depthBuffer);
	inline FramebufferTexture* GetColorAttachment() const { return m_ColorAttachment; };
	inline FramebufferTexture* GetDepthAttachment() const { return m_DepthAttachment; };
	inline Renderbuffer* GetDepthBuffer() const { return m_DepthBuffer; };
	inline unsigned int GetID() const { return m_FBO; };
	void Bind();
	void Unbind();
	~FramebufferWater();

private:
	unsigned int m_FBO = -1;
	unsigned int m_Width = -1;
	unsigned int m_Height = -1;

	FramebufferTexture* m_ColorAttachment = nullptr;
	FramebufferTexture* m_DepthAttachment = nullptr;
	Renderbuffer* m_DepthBuffer = nullptr;
	FramebufferTexture* m_StencilAttachment = nullptr; // still not in use

};
