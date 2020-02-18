#pragma once

#include "Light.h"


class DirectionalLight : public Light
{

public:
	DirectionalLight();

	DirectionalLight(GLfloat red, GLfloat green, GLfloat blue,
		GLfloat ambIntensity, GLfloat diffIntensity,
		GLfloat directionX, GLfloat directionY, GLfloat directionZ);

	void UseLight(GLint ambientIntensityLocation, GLint ambientColorLocation,
		GLint diffuseIntensityLocation, GLint diffuseDirectionLocation);

	~DirectionalLight();

private:
	glm::vec3 direction;


};
