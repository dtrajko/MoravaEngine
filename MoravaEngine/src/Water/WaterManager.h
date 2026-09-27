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
	FramebufferWater* m_ReflectionFB = nullptr;
	FramebufferWater* m_RefractionFB = nullptr;

	int m_Width = 0;
	int m_Height = 0;

	float m_WaterHeight = 0.0f;
	float m_MoveFactor = 0.0f;

	glm::vec4 m_WaterColor = glm::vec4(0.0f, 0.6f, 1.0f, 1.0f);

};
