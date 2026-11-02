#pragma once

#include "Light/Light.h"

#include <vector>


class PointLight : public Light
{
public:
	PointLight();
	PointLight(GLuint shadowWidth, GLuint shadowHeight, GLfloat nearPlane, GLfloat farPlane,
		bool enabled, glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity,
		glm::vec3 position, GLfloat constant, GLfloat linear, GLfloat exponent);
	~PointLight();

	void UseLight(GLint enabledLocation, GLint ambientColorLocation, GLint ambientIntensityLocation, GLint diffuseIntensityLocation,
		GLint positionLocation, GLint constantLocation, GLint linearLocation, GLint exponentLocation);
	std::vector<glm::mat4> CalculateLightTransform();
	inline GLfloat GetFarPlane() const { return m_FarPlane; };
	inline glm::vec3 GetPosition() const { return m_Position; };
	inline void SetPosition(glm::vec3 position) { m_Position = position; };
	inline float GetAmbientIntensity() const { return m_AmbientIntensity; };
	inline void SetAmbientIntensity(float ambientIntensity) { m_AmbientIntensity = ambientIntensity; };
	inline const float GetConstant() const { return m_Constant; };
	inline const float GetLinear() const { return m_Linear; };
	inline const float GetExponent() const { return m_Exponent; };
	inline void SetConstant(float constant) { m_Constant = constant; };
	inline void SetLinear(float linear) { m_Linear = linear; };
	inline void SetExponent(float exponent) { m_Exponent = exponent; };

protected:
	glm::vec3 m_Position;
	GLfloat m_Constant;
	GLfloat m_Linear;
	GLfloat m_Exponent;

	GLfloat m_FarPlane;

};
