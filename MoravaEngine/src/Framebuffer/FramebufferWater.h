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

	void AddColorAttachment(Hazel::Ref<FramebufferTexture> colorAttachment);
	void AddDepthAttachment(Hazel::Ref<FramebufferTexture> depthAttachment);
	void AddDepthBuffer(Hazel::Ref<Renderbuffer> depthBuffer);
	inline Hazel::Ref<FramebufferTexture> GetColorAttachment() const { return m_ColorAttachment; };
	inline Hazel::Ref<FramebufferTexture> GetDepthAttachment() const { return m_DepthAttachment; };
	inline Hazel::Ref<Renderbuffer> GetDepthBuffer() const { return m_DepthBuffer; };
	inline unsigned int GetID() const { return m_FBO; };
	void Bind();
	void Unbind();

private:
	unsigned int m_FBO = -1;
	unsigned int m_Width = -1;
	unsigned int m_Height = -1;

	Hazel::Ref<FramebufferTexture> m_ColorAttachment = nullptr;
	Hazel::Ref<FramebufferTexture> m_DepthAttachment = nullptr;

	Hazel::Ref<Renderbuffer> m_DepthBuffer = nullptr;
	Hazel::Ref<FramebufferTexture> m_StencilAttachment = nullptr; // still not in use

};
