#include "ShaderLearnOpenGL.h"



ShaderLearnOpenGL::ShaderLearnOpenGL() : Shader()
{
}

void ShaderLearnOpenGL::GetUniformLocations()
{
	printf("ShaderLearnOpenGL::GetUniformLocations\n");
	Shader::GetUniformLocations();
}

ShaderLearnOpenGL::~ShaderLearnOpenGL()
{
}
