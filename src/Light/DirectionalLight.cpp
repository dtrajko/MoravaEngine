#include "Light/DirectionalLight.h"

#include <glm/gtc/matrix_transform.hpp>


DirectionalLight::DirectionalLight()
	: Light()
{
	m_Direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
	m_LightProj = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);

	m_Enabled = true;
}

DirectionalLight::DirectionalLight(GLuint shadowWidth, GLuint shadowHeight,
	bool enabled, glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity, glm::vec3 direction, bool createShadowMap)
	: Light(shadowWidth, shadowHeight, color, ambientIntensity, diffuseIntensity)
{
	if (createShadowMap) {
		m_ShadowMap = new ShadowMap();
		m_ShadowMap->Init(shadowWidth, shadowHeight);
	}

	m_Direction = glm::normalize(direction);
	m_LightProj = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);

	m_Enabled = enabled;
}

void DirectionalLight::UseLight(GLint enabledLocation, GLint ambientColorLocation,
	GLint ambientIntensityLocation, GLint diffuseIntensityLocation, GLint directionLocation)
{
	glUniform1i(enabledLocation, m_Enabled);
	glUniform3f(ambientColorLocation, m_Color.x, m_Color.y, m_Color.z);
	glUniform1f(ambientIntensityLocation, m_AmbientIntensity);
	glUniform1f(diffuseIntensityLocation, m_DiffuseIntensity);
	glUniform3f(directionLocation, m_Direction.x, m_Direction.y, m_Direction.z);
}

void DirectionalLight::SetDirection(glm::vec3 direction)
{
	m_Direction = glm::normalize(direction);
}

glm::mat4 DirectionalLight::CalculateLightTransform()
{
	return m_LightProj * glm::lookAt(-m_Direction, glm::vec3(0.0f, -m_Direction.y * 0.75f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

DirectionalLight::~DirectionalLight()
{
}
