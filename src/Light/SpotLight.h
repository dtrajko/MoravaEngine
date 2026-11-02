#pragma once

#include "Light/PointLight.h"
#include "Light/DirectionalLight.h"


class SpotLight : public DirectionalLight, PointLight
{
public:
	SpotLight();
	SpotLight(GLuint shadowWidth, GLuint shadowHeight, GLfloat nearPlane, GLfloat farPlane,
		bool enabled, glm::vec3 color, GLfloat ambientIntensity, GLfloat diffuseIntensity,
		glm::vec3 position, glm::vec3 direction,
		GLfloat constant, GLfloat linear, GLfloat exponent, GLfloat edge);
	~SpotLight();

	void UseLight(GLint enabledLocation, GLint ambientColorLocation, GLint ambientIntensityLocation, GLint diffuseIntensityLocation,
		GLint positionLocation, GLint directionLocation,
		GLint constantLocation, GLint linearLocation, GLint exponentLocation,
		GLint edgeLocation);
	inline ShadowMap* GetShadowMap() { return PointLight::GetShadowMap(); };
	inline GLfloat GetFarPlane() { return PointLight::GetFarPlane(); };
	void SetFlash(glm::vec3 position, glm::vec3 direction);
	PointLight* const GetBasePL() const { return (PointLight*)this; }
	inline const float GetEdge() const { return m_Edge; };
	void SetEdge(float edge);

private:
	GLfloat m_Edge;
	GLfloat m_EdgeProcessed;

};
