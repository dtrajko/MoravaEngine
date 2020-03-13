#pragma once

#include "Framebuffer.h"
#include "Renderbuffer.h"
#include "TextureCubemap.h"


class Cubemap
{
public:
	Cubemap();
	Cubemap(int width, int height);
	~Cubemap();

private:
	unsigned int m_Width;
	unsigned int m_Height;

	Framebuffer* m_CaptureFBO;
	Renderbuffer* m_CaptureRBO;
	TextureCubemap* m_TextureCubemap;

};
