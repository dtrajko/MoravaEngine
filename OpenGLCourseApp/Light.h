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
	ShadowMap* GetShadowMap() { return shadowMap; };
	~Light();

protected:
	glm::vec3 m_Color;
	GLfloat m_AmbientIntensity;
	GLfloat m_DiffuseIntensity;

	glm::mat4 lightProj;
	ShadowMap* shadowMap;

};
