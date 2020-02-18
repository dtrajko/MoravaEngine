#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>



class Light
{
public:
	Light();
	Light(glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity);
	~Light();

protected:
	glm::vec3 m_Color;
	GLfloat m_AmbientIntensity;
	GLfloat m_DiffuseIntensity;

};
