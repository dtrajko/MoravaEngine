#pragma once

#include "Framebuffer/ShadowMap.h"

#include <GL/glew.h>
#include <glm/glm.hpp>


class Light
{
public:
	Light();
	Light(GLuint shadowWidth, GLuint shadowHeight,
		glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity);
	ShadowMap* GetShadowMap() { return m_ShadowMap; };
	inline void SetLightProjection(glm::mat4 lightProj) { m_LightProj = lightProj; };
	inline glm::vec3 GetColor() const { return m_Color; }
	inline void SetColor(glm::vec3 color) { m_Color = color; }
	inline float GetAmbientIntensity() const { return m_AmbientIntensity; }
	inline void SetAmbientIntensity(float ambientIntensity) { m_AmbientIntensity = ambientIntensity; }
	inline float GetDiffuseIntensity() const { return m_DiffuseIntensity; }
	inline void SetDiffuseIntensity(float diffuseIntensity) { m_DiffuseIntensity = diffuseIntensity; }
	inline const bool GetEnabled() const { return m_Enabled; };
	inline void SetEnabled(bool enabled) { m_Enabled = enabled; }
	void Toggle() { m_Enabled = !m_Enabled; };
	~Light();

protected:
	bool m_Enabled;
	glm::vec3 m_Color;
	GLfloat m_AmbientIntensity;
	GLfloat m_DiffuseIntensity;

	glm::mat4 m_LightProj;
	ShadowMap* m_ShadowMap;
};
