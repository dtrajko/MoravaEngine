#pragma once

#include "Light.h"


class PointLight : public Light
{
public:
	PointLight();
	PointLight(glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity,
		glm::vec3 position, GLfloat constant, GLfloat linear, GLfloat exponent);
	void UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation, GLint diffuseIntensityLocation,
		GLint positionLocation, GLint constantLocation, GLint linearLocation, GLint exponentLocation);
	~PointLight();

protected:
	glm::vec3 m_Position;
	GLfloat m_Constant;
	GLfloat m_Linear;
	GLfloat m_Exponent;

};
