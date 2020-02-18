#include "Light.h"


Light::Light()
{
	m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
	m_AmbientIntensity = 1.0f;
	m_DiffuseIntensity = 0.0f;
}

Light::Light(glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity)
{
	m_Color = glm::vec3(color.r, color.g, color.b);
	m_AmbientIntensity = ambientIntensity;
	m_DiffuseIntensity = diffuseIntensity;
}

Light::~Light()
{
}
