#pragma once

#include "Shader.h"


class ShaderCubemap : public Shader
{
public:
	ShaderCubemap();

	GLint GetUniformLocationEquirectangularMap();

	void SetEquirectangularMap(int equirectangularMap);

	~ShaderCubemap();

private:
	virtual void GetUniformLocations() override;

private:
	GLint uniformEquirectangularMap = -1;

};
