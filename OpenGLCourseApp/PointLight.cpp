#include "PointLight.h"



PointLight::PointLight()
	: Light()
{
	m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Constant = 1.0f;
	m_Linear = 0.0f;
	m_Exponent = 0.0f;
}

PointLight::PointLight(glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity,
	glm::vec3 position, GLfloat constant, GLfloat linear, GLfloat exponent)
	: Light(color, ambientIntensity, diffuseIntensity)
{
	m_Position = position;
	m_Constant = constant;
	m_Linear = linear;
	m_Exponent = exponent;
}

void PointLight::UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation, GLint diffuseIntensityLocation,
	GLint positionLocation, GLint constantLocation, GLint linearLocation, GLint exponentLocation)
{
	glUniform3f(ambientColorLocation, m_Color.r, m_Color.g, m_Color.b);
	glUniform1f(ambientIntensityLocation, m_AmbientIntensity);
	glUniform1f(diffuseIntensityLocation, m_DiffuseIntensity);
	glUniform3f(positionLocation, m_Position.x, m_Position.y, m_Position.z);
	glUniform1f(constantLocation, m_Constant);
	glUniform1f(linearLocation, m_Linear);
	glUniform1f(exponentLocation, m_Exponent);
}

PointLight::~PointLight()
{
}
