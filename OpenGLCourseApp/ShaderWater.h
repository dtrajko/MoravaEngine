#pragma once

#include "Shader.h"


class ShaderWater : public Shader
{
public:
	ShaderWater();

	GLint GetUniformLocationReflectionTexture();
	GLint GetUniformLocationRefractionTexture();
	GLint GetUniformLocationDuDvMap();
	GLint GetUniformLocationMoveFactor();
	GLint GetUniformLocationCameraPosition();
	GLint GetUniformLocationLightColor();

	// Water shader samplers
	void SetWater(unsigned int txUnitReflection, unsigned int txUnitRefraction,
		unsigned int txUnitDuDv, unsigned int txUnitDepth);
	void SetMoveFactor(float moveFactor);
	void SetCameraPosition(glm::vec3 cameraPosition);
	void SetLightColor(glm::vec3 lightColor);
	void SetDuDvMap(GLuint textureUnit);

	~ShaderWater();

private:
	virtual void GetUniformLocations() override;

private:
	bool m_Validated = false;

	GLuint uniformMoveFactor;
	GLuint uniformReflectionTexture;
	GLuint uniformRefractionTexture;
	GLuint uniformDuDvMap;
	GLuint uniformCameraPosition;
	GLuint uniformLightColor;
};
