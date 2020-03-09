#pragma once

#include "Light.h"

#include <vector>


class PointLight : public Light
{
public:
	PointLight();
	PointLight(GLuint shadowWidth, GLuint shadowHeight, GLfloat nearPlane, GLfloat farPlane,
		glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity,
		glm::vec3 position, GLfloat constant, GLfloat linear, GLfloat exponent);
	void UseLight(GLint ambientColorLocation, GLint ambientIntensityLocation, GLint diffuseIntensityLocation,
		GLint positionLocation, GLint constantLocation, GLint linearLocation, GLint exponentLocation);
	std::vector<glm::mat4> CalculateLightTransform();
	inline GLfloat GetFarPlane() const { return m_FarPlane; };
	inline glm::vec3 GetPosition() const { return m_Position; };
	inline void SetPosition(glm::vec3 position) { m_Position = position; };
	inline float GetAmbientIntensity() const { return m_AmbientIntensity; }
	inline void SetAmbientIntensity(float ambientIntensity) { m_AmbientIntensity = ambientIntensity; }
	~PointLight();

protected:
	glm::vec3 m_Position;
	GLfloat m_Constant;
	GLfloat m_Linear;
	GLfloat m_Exponent;

	GLfloat m_FarPlane;

};
