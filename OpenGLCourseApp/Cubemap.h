#pragma once

#include "Framebuffer.h"
#include "Renderbuffer.h"
#include "TextureCubemap.h"


class Cubemap
{
public:
	Cubemap();
	Cubemap(int width, int height);
	inline TextureCubemap* GetTextureCubemap() { return m_TextureCubemap; };
	inline Framebuffer* GetCaptureFBO() { return m_CaptureFBO; };
	inline Renderbuffer* GetCaptureRBO() { return m_CaptureRBO; };
	~Cubemap();

private:
	unsigned int m_Width;
	unsigned int m_Height;

	Framebuffer* m_CaptureFBO;
	Renderbuffer* m_CaptureRBO;
	TextureCubemap* m_TextureCubemap;

};
