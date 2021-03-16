#include "PointLight.h"

#include "Framebuffer/OmniShadowMap.h"

#include <glm/gtc/matrix_transform.hpp>


PointLight::PointLight()
	: Light()
{
	m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Constant = 1.0f;
	m_Linear = 0.0f;
	m_Exponent = 0.0f;

	m_FarPlane = 100.0f;

	m_Enabled = false;
}

PointLight::PointLight(GLuint shadowWidth, GLuint shadowHeight, GLfloat nearPlane, GLfloat farPlane,
	bool enabled, glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity,
	glm::vec3 position, GLfloat constant, GLfloat linear, GLfloat exponent)
	: Light(shadowWidth, shadowHeight, color, ambientIntensity, diffuseIntensity)
{
	m_Enabled = enabled;

	m_Position = position;
	m_Constant = constant;
	m_Linear = linear;
	m_Exponent = exponent;

	m_FarPlane = farPlane;

	float aspect = (float)shadowWidth / (float)shadowHeight;
	m_LightProj = glm::perspective(glm::radians(90.0f), aspect, nearPlane, farPlane);

	m_ShadowMap = new OmniShadowMap();
	m_ShadowMap->Init(shadowWidth, shadowHeight);
}

void PointLight::UseLight(GLint enabledLocation, GLint ambientColorLocation, GLint ambientIntensityLocation, GLint diffuseIntensityLocation,
	GLint positionLocation, GLint constantLocation, GLint linearLocation, GLint exponentLocation)
{
	glUniform1i(enabledLocation, m_Enabled);
	glUniform3f(ambientColorLocation, m_Color.r, m_Color.g, m_Color.b);
	glUniform1f(ambientIntensityLocation, m_AmbientIntensity);
	glUniform1f(diffuseIntensityLocation, m_DiffuseIntensity);
	glUniform3f(positionLocation, m_Position.x, m_Position.y, m_Position.z);
	glUniform1f(constantLocation, m_Constant);
	glUniform1f(linearLocation, m_Linear);
	glUniform1f(exponentLocation, m_Exponent);
}

std::vector<glm::mat4> PointLight::CalculateLightTransform()
{
	std::vector<glm::mat4> lightMatrices = std::vector<glm::mat4>();
	// X axis
	lightMatrices.push_back(m_LightProj * glm::lookAt(m_Position, m_Position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	lightMatrices.push_back(m_LightProj * glm::lookAt(m_Position, m_Position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	// Y axis
	lightMatrices.push_back(m_LightProj * glm::lookAt(m_Position, m_Position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	lightMatrices.push_back(m_LightProj * glm::lookAt(m_Position, m_Position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
	// Z axis
	lightMatrices.push_back(m_LightProj * glm::lookAt(m_Position, m_Position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	lightMatrices.push_back(m_LightProj * glm::lookAt(m_Position, m_Position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

	return lightMatrices;
}

PointLight::~PointLight()
{
}
