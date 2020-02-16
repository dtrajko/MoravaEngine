#include "Light.h"


Light::Light()
{
	color = glm::vec3(1.0f, 1.0f, 1.0f);
	ambientIntensity = 1.0f;

	direction = glm::vec3(0.0f, -1.0f, 0.0f);
	diffuseIntensity = 0.0f;
}

Light::Light(GLfloat red, GLfloat green, GLfloat blue, GLfloat ambIntensity,
	GLfloat directionX, GLfloat directionY, GLfloat directionZ, GLfloat diffIntensity)
{
	color = glm::vec3(red, green, blue);
	ambientIntensity = ambIntensity;
	direction = glm::vec3(directionX, directionY, directionZ);
	diffuseIntensity = diffIntensity;
}

void Light::UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation,
	GLint diffuseDirectionLocation, GLint diffuseIntensityLocation)
{
	glUniform3f(ambientColorLocation, color.x, color.y, color.z);
	glUniform1f(ambientIntensityLocation, ambientIntensity);

	glUniform3f(diffuseDirectionLocation, direction.x, direction.y, direction.z);
	glUniform1f(diffuseIntensityLocation, diffuseIntensity);
}

Light::~Light()
{
}
