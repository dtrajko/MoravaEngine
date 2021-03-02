#pragma once

#include "Framebuffer/FramebufferWater.h"
#include "Framebuffer/Renderbuffer.h"
#include "Texture/TextureCubemap.h"


class Cubemap
{
public:
	Cubemap();
	Cubemap(int width, int height);
	inline TextureCubemap* GetTextureCubemap() { return m_TextureCubemap; };
	inline FramebufferWater* GetCaptureFBO() { return m_CaptureFBO; };
	inline Renderbuffer* GetCaptureRBO() { return m_CaptureRBO; };
	~Cubemap();

private:
	unsigned int m_Width;
	unsigned int m_Height;

	FramebufferWater* m_CaptureFBO;
	Renderbuffer* m_CaptureRBO;
	TextureCubemap* m_TextureCubemap;

};
