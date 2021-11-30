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

	void AddColorAttachment(H2M::Ref<FramebufferTexture> colorAttachment);
	void AddDepthAttachment(H2M::Ref<FramebufferTexture> depthAttachment);
	void AddDepthBuffer(H2M::Ref<Renderbuffer> depthBuffer);
	inline H2M::Ref<FramebufferTexture> GetColorAttachment() const { return m_ColorAttachment; };
	inline H2M::Ref<FramebufferTexture> GetDepthAttachment() const { return m_DepthAttachment; };
	inline H2M::Ref<Renderbuffer> GetDepthBuffer() const { return m_DepthBuffer; };
	inline unsigned int GetID() const { return m_FBO; };
	void Bind();
	void Unbind();

private:
	unsigned int m_FBO = -1;
	unsigned int m_Width = -1;
	unsigned int m_Height = -1;

	H2M::Ref<FramebufferTexture> m_ColorAttachment = H2M::Ref<FramebufferTexture>();
	H2M::Ref<FramebufferTexture> m_DepthAttachment = H2M::Ref<FramebufferTexture>();

	H2M::Ref<Renderbuffer> m_DepthBuffer = H2M::Ref<Renderbuffer>();
	H2M::Ref<FramebufferTexture> m_StencilAttachment = H2M::Ref<FramebufferTexture>(); // still not in use

};

