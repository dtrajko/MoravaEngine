#include "DirectionalLight.h"



DirectionalLight::DirectionalLight()
	: Light()
{
	direction = glm::vec3(0.0f, -1.0f, 0.0f);
}

DirectionalLight::DirectionalLight(GLfloat red, GLfloat green, GLfloat blue,
	GLfloat ambIntensity, GLfloat diffIntensity,
	GLfloat directionX, GLfloat directionY, GLfloat directionZ)
	: Light(red, green, blue, ambIntensity, diffIntensity)
{
	direction = glm::vec3(directionX, directionY, directionZ);
}

void DirectionalLight::UseLight(GLint ambientIntensityLocation, GLint ambientColorLocation, 
	GLint diffuseIntensityLocation, GLint diffuseDirectionLocation)
{
	glUniform1f(ambientIntensityLocation, ambientIntensity);
	glUniform3f(ambientColorLocation, color.x, color.y, color.z);
	glUniform1f(diffuseIntensityLocation, diffuseIntensity);
	glUniform3f(diffuseDirectionLocation, direction.x, direction.y, direction.z);
}

DirectionalLight::~DirectionalLight()
{
}
