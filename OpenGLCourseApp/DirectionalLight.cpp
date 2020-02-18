#include "DirectionalLight.h"



DirectionalLight::DirectionalLight()
	: Light()
{
	m_Direction = glm::vec3(0.0f, -1.0f, 0.0f);
}

DirectionalLight::DirectionalLight(glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity, glm::vec3 direction)
	: Light(color, ambientIntensity, diffuseIntensity)
{
	m_Direction = direction;
}

void DirectionalLight::UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation,
	GLint diffuseIntensityLocation, GLint directionLocation)
{
	glUniform3f(ambientColorLocation, m_Color.x, m_Color.y, m_Color.z);
	glUniform1f(ambientIntensityLocation, m_AmbientIntensity);
	glUniform1f(diffuseIntensityLocation, m_DiffuseIntensity);
	glUniform3f(directionLocation, m_Direction.x, m_Direction.y, m_Direction.z);
}

DirectionalLight::~DirectionalLight()
{
}
