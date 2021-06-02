#pragma once

#include "Shader/MoravaShader.h"


class ShaderPBR : public MoravaShader
{
public:
	ShaderPBR();
	void SetLightPositions(glm::vec3* lightPositions, unsigned int lightCount);
	void SetLightColors(glm::vec3* lightColors, unsigned int lightCount);
	~ShaderPBR();

private:
	virtual void GetUniformLocations() override;

private:
	static const unsigned int lightCount = 4;

	// lights
	GLint uniformLightPositions[lightCount];
	GLint uniformLightColors[lightCount];

};
