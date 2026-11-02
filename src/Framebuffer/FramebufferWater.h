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
	~FramebufferWater();

	void AddColorAttachment(H2M::RefH2M<FramebufferTexture> colorAttachment);
	void AddDepthAttachment(H2M::RefH2M<FramebufferTexture> depthAttachment);
	void AddDepthBuffer(H2M::RefH2M<Renderbuffer> depthBuffer);
	inline H2M::RefH2M<FramebufferTexture> GetColorAttachment() const { return m_ColorAttachment; };
	inline H2M::RefH2M<FramebufferTexture> GetDepthAttachment() const { return m_DepthAttachment; };
	inline H2M::RefH2M<Renderbuffer> GetDepthBuffer() const { return m_DepthBuffer; };
	inline unsigned int GetID() const { return m_FBO; };
	void Bind();
	void Unbind();

private:
	unsigned int m_FBO = -1;
	unsigned int m_Width = -1;
	unsigned int m_Height = -1;

	H2M::RefH2M<FramebufferTexture> m_ColorAttachment = H2M::RefH2M<FramebufferTexture>();
	H2M::RefH2M<FramebufferTexture> m_DepthAttachment = H2M::RefH2M<FramebufferTexture>();

	H2M::RefH2M<Renderbuffer> m_DepthBuffer = H2M::RefH2M<Renderbuffer>();
	H2M::RefH2M<FramebufferTexture> m_StencilAttachment = H2M::RefH2M<FramebufferTexture>(); // still not in use

};

