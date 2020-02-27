#pragma once

#include "Framebuffer.h"


class WaterManager
{
public:
	WaterManager();
	WaterManager(int width, int height);
	void SwitchToDefaultFramebuffer();
	inline int GetFramebufferWidth() const { return m_Width; };
	inline int GetFramebufferHeight() const { return m_Height; };
	inline Framebuffer* GetReflectionFramebuffer() const { return reflectionFB; };
	inline Framebuffer* GetRefractionFramebuffer() const { return refractionFB; };
	~WaterManager();

private:
	Framebuffer* reflectionFB;
	Framebuffer* refractionFB;

	int m_Width;
	int m_Height;
};
