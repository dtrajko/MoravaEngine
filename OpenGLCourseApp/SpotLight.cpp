#include "SpotLight.h"


SpotLight::SpotLight()
	: DirectionalLight(), PointLight()
{
	m_Edge = 0.0f;
	m_EdgeProcessed = cosf(glm::radians(m_Edge));
}

SpotLight::SpotLight(glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity,
	glm::vec3 position, glm::vec3 direction,
	GLfloat constant, GLfloat linear, GLfloat exponent, GLfloat edge)
	: DirectionalLight(color, ambientIntensity, diffuseIntensity, direction),
	PointLight(color, ambientIntensity, diffuseIntensity, position, constant, linear, exponent)
{
	m_Edge = edge;
	m_EdgeProcessed = cosf(glm::radians(m_Edge));
}

void SpotLight::UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation, GLint diffuseIntensityLocation,
	GLint positionLocation, GLint directionLocation,
	GLint constantLocation, GLint linearLocation, GLint exponentLocation,
	GLint edgeLocation)
{
	glUniform3f(ambientColorLocation, Light::m_Color.r, Light::m_Color.g, Light::m_Color.b);
	glUniform1f(ambientIntensityLocation, Light::m_AmbientIntensity);
	glUniform1f(diffuseIntensityLocation, Light::m_DiffuseIntensity);
	glUniform3f(positionLocation, m_Position.x, m_Position.y, m_Position.z);
	glUniform3f(directionLocation, m_Direction.x, m_Direction.y, m_Direction.z);
	glUniform1f(constantLocation, m_Constant);
	glUniform1f(linearLocation, m_Linear);
	glUniform1f(exponentLocation, m_Exponent);
	glUniform1f(edgeLocation, m_EdgeProcessed);
}

void SpotLight::SetFlash(glm::vec3 position, glm::vec3 direction)
{
	m_Position = position;
	m_Direction = direction;
}

SpotLight::~SpotLight()
{

}
