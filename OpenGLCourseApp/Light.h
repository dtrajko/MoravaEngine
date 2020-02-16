#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>


class Light
{
public:
	Light();
	Light(GLfloat red, GLfloat green, GLfloat blue, GLfloat ambIntensity,
		GLfloat directionX, GLfloat directionY, GLfloat directionZ, GLfloat diffIntensity);
	void UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation,
		GLint diffuseDirectionLocation, GLint diffuseIntensityLocation);
	~Light();

private:
	glm::vec3 color;
	GLfloat ambientIntensity;

	glm::vec3 direction;
	GLfloat diffuseIntensity;

};
