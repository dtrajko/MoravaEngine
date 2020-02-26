#pragma once

#include "Framebuffer.h"


class WaterManager
{
public:
	WaterManager();
	WaterManager(int width, int height);
	~WaterManager();

private:
	Framebuffer* reflectionFB;
	Framebuffer* refractionFB;

	int m_Width;
	int m_Height;
};
