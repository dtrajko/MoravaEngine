#pragma once

#include "Light.h"


class DirectionalLight : public Light
{

public:
	DirectionalLight();

	DirectionalLight(glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity, glm::vec3 direction);

	void UseLight(GLint ambientIntensityLocation, GLint ambientColorLocation,
		GLint diffuseIntensityLocation, GLint diffuseDirectionLocation);

	~DirectionalLight();

private:
	glm::vec3 m_Direction;


};
