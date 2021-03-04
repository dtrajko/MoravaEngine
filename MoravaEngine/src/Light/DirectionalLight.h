#pragma once

#include "Light/Light.h"


class DirectionalLight : public Light
{

public:
	DirectionalLight();

	DirectionalLight(GLuint shadowWidth, GLuint shadowHeight,
		bool enabled, glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity, glm::vec3 direction, bool createShadowMap);
	void UseLight(GLint enabledLocation, GLint ambientColorLocation, GLint ambientIntensityLocation,
		GLint diffuseIntensityLocation, GLint directionLocation);
	inline glm::vec3 GetDirection() const { return glm::normalize(m_Direction); };
	inline glm::vec3 GetPosition() const { return -glm::normalize(m_Direction); };
	void SetDirection(glm::vec3 direction);
	glm::mat4 CalculateLightTransform();

	~DirectionalLight();

private:
	glm::vec3 m_Direction;

};
