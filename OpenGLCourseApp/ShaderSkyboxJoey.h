#pragma once

#include "Shader.h"


class ShaderSkyboxJoey : public Shader
{
public:
	ShaderSkyboxJoey();

	GLint GetUniformLocationEnvironmentMap();

	void SetEnvironmentMap(int environmentMap);

	~ShaderSkyboxJoey();

private:
	virtual void GetUniformLocations() override;

private:
	GLint uniformEnvironmentMap = -1;

};
