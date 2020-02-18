#pragma once

#include "Light.h"


class DirectionalLight : public Light
{

public:
	DirectionalLight();

	DirectionalLight(glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity, glm::vec3 direction);

	void UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation,
		GLint diffuseIntensityLocation, GLint directionLocation);

	~DirectionalLight();

private:
	glm::vec3 m_Direction;


};
