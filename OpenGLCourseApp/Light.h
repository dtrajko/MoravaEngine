#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "ShadowMap.h"


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
	~Light();

protected:
	glm::vec3 m_Color;
	GLfloat m_AmbientIntensity;
	GLfloat m_DiffuseIntensity;

	glm::mat4 m_LightProj;
	ShadowMap* m_ShadowMap;
};
