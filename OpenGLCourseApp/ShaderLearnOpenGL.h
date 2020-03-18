#pragma once

#include "Shader.h"


class ShaderLearnOpenGL : public Shader
{
public:
	ShaderLearnOpenGL();
	ShaderLearnOpenGL(const char* vertexLocation, const char* fragmentLocation);
	void GetUniformLocations();
	~ShaderLearnOpenGL();

};
