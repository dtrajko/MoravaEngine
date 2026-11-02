#include "Light/SpotLight.h"


SpotLight::SpotLight()
	: DirectionalLight(), PointLight()
{
	m_Edge = 0.0f;
	m_EdgeProcessed = cosf(glm::radians(m_Edge));

	PointLight::m_Enabled = false;
}

SpotLight::SpotLight(GLuint shadowWidth, GLuint shadowHeight, GLfloat nearPlane, GLfloat farPlane,
	bool enabled, glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity,
	glm::vec3 position, glm::vec3 direction,
	GLfloat constant, GLfloat linear, GLfloat exponent, GLfloat edge)
	: DirectionalLight(shadowWidth, shadowHeight, enabled, color, ambientIntensity, diffuseIntensity, direction, false),
	PointLight(shadowWidth, shadowHeight, nearPlane, farPlane, enabled, color, ambientIntensity, diffuseIntensity,
		position, constant, linear, exponent)
{
	m_Edge = edge;
	m_EdgeProcessed = cosf(glm::radians(m_Edge));

	PointLight::m_Enabled = enabled;
}

void SpotLight::UseLight(GLint enabledLocation, GLint ambientColorLocation, GLint ambientIntensityLocation, GLint diffuseIntensityLocation,
	GLint positionLocation, GLint directionLocation,
	GLint constantLocation, GLint linearLocation, GLint exponentLocation,
	GLint edgeLocation)
{
	glUniform1i(enabledLocation, PointLight::m_Enabled);
	glUniform3f(ambientColorLocation, PointLight::m_Color.r, PointLight::m_Color.g, PointLight::m_Color.b);
	glUniform1f(ambientIntensityLocation, PointLight::m_Enabled ? PointLight::m_AmbientIntensity : 0.0f);
	glUniform1f(diffuseIntensityLocation, PointLight::m_Enabled ? PointLight::m_DiffuseIntensity : 0.0f);
	glUniform3f(positionLocation, m_Position.x, m_Position.y, m_Position.z);
	glUniform3f(directionLocation, GetDirection().x, GetDirection().y, GetDirection().z);
	glUniform1f(constantLocation, m_Constant);
	glUniform1f(linearLocation, m_Linear);
	glUniform1f(exponentLocation, m_Exponent);
	glUniform1f(edgeLocation, m_EdgeProcessed);
}

void SpotLight::SetFlash(glm::vec3 position, glm::vec3 direction)
{
	m_Position = position;
	SetDirection(direction);
}

void SpotLight::SetEdge(float edge)
{
	m_Edge = edge;
	m_EdgeProcessed = cosf(glm::radians(m_Edge));
}

SpotLight::~SpotLight()
{

}
