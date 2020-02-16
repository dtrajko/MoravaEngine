#include "Light.h"


Light::Light()
{
	color = glm::vec3(1.0f, 1.0f, 1.0f);
	ambientIntensity = 1.0f;
}

Light::Light(GLfloat red, GLfloat green, GLfloat blue, GLfloat ambIntensity)
{
	color = glm::vec3(red, green, blue);
	ambientIntensity = ambIntensity;
}

void Light::UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation)
{
	glUniform3f(ambientColorLocation, color.x, color.y, color.z);
	glUniform1f(ambientIntensityLocation, ambientIntensity);
}

Light::~Light()
{
}
