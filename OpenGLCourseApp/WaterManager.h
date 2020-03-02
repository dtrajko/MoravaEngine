#pragma once

#include "Framebuffer.h"


class WaterManager
{
public:
	WaterManager();
	WaterManager(int width, int height, float waterHeight, float waveSpeed);
	void SwitchToDefaultFramebuffer();
	inline int GetFramebufferWidth() const { return m_Width; };
	inline int GetFramebufferHeight() const { return m_Height; };
	inline Framebuffer* GetReflectionFramebuffer() const { return reflectionFB; };
	inline Framebuffer* GetRefractionFramebuffer() const { return refractionFB; };
	inline float GetWaterHeight() const { return m_WaterHeight; };
	inline void SetWaterHeight(float waterHeight) { m_WaterHeight = waterHeight; };
	inline float GetWaterMoveFactor() const { return m_WaterMoveFactor; };
	inline void SetWaterMoveFactor(float waterMoveFactor) { m_WaterMoveFactor = waterMoveFactor; };
	~WaterManager();

public:
	static float m_WaveSpeed;

private:
	Framebuffer* reflectionFB;
	Framebuffer* refractionFB;

	int m_Width;
	int m_Height;

	float m_WaterHeight;
	float m_WaterMoveFactor;
};
