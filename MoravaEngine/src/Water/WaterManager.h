#pragma once

#include "Framebuffer/FramebufferWater.h"

#include "glm/glm.hpp"


class WaterManager
{
public:
	WaterManager();
	WaterManager(int width, int height, float waterHeight, float waveSpeed);
	~WaterManager();

	void SwitchToDefaultFramebuffer();
	inline int GetFramebufferWidth() const { return m_Width; };
	inline int GetFramebufferHeight() const { return m_Height; };
	inline FramebufferWater* GetReflectionFramebuffer() const { return m_ReflectionFB; };
	inline FramebufferWater* GetRefractionFramebuffer() const { return m_RefractionFB; };
	inline float GetWaterHeight() const { return m_WaterHeight; };
	inline void SetWaterHeight(float waterHeight) { m_WaterHeight = waterHeight; };
	inline glm::vec4 GetWaterColor() const { return m_WaterColor; };
	inline void SetWaterColor(glm::vec4 waterColor) { m_WaterColor = waterColor; };
	inline float GetWaterMoveFactor() const { return m_MoveFactor; };
	inline void SetWaterMoveFactor(float moveFactor) { m_MoveFactor = moveFactor; };
	inline float GetWaveSpeed() const { return m_WaveSpeed; };
	inline void SetWaveSpeed(float waveSpeed) { m_WaveSpeed = waveSpeed; };

public:
	static float m_WaveSpeed;

private:
	FramebufferWater* m_ReflectionFB;
	FramebufferWater* m_RefractionFB;

	int m_Width;
	int m_Height;

	float m_WaterHeight;
	float m_MoveFactor;

	glm::vec4 m_WaterColor;

};
