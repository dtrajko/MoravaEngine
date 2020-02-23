#include "DirectionalLight.h"

#include <glm/gtc/matrix_transform.hpp>



DirectionalLight::DirectionalLight()
	: Light()
{
	m_Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	lightProj = glm::ortho(-200.0f, 200.0f, -200.0f, 200.0f, 0.1f, 200.0f);
}

DirectionalLight::DirectionalLight(GLuint shadowWidth, GLuint shadowHeight,
	glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity, glm::vec3 direction)
	: Light(shadowWidth, shadowHeight, color, ambientIntensity, diffuseIntensity)
{
	m_Direction = glm::normalize(direction);
	lightProj = glm::ortho(-200.0f, 200.0f, -200.0f, 200.0f, 0.1f, 200.0f);
}

void DirectionalLight::UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation,
	GLint diffuseIntensityLocation, GLint directionLocation)
{
	glUniform3f(ambientColorLocation, m_Color.x, m_Color.y, m_Color.z);
	glUniform1f(ambientIntensityLocation, m_AmbientIntensity);
	glUniform1f(diffuseIntensityLocation, m_DiffuseIntensity);
	glUniform3f(directionLocation, m_Direction.x, m_Direction.y, m_Direction.z);
}

glm::mat4 DirectionalLight::CalculateLightTransform()
{
	return lightProj * glm::lookAt(-m_Direction, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

DirectionalLight::~DirectionalLight()
{
}
