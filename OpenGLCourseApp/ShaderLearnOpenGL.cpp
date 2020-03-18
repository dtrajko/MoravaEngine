#include "ShaderLearnOpenGL.h"



ShaderLearnOpenGL::ShaderLearnOpenGL() : Shader()
{
}

ShaderLearnOpenGL::ShaderLearnOpenGL(const char* vertexLocation, const char* fragmentLocation)
	: Shader(vertexLocation, fragmentLocation)
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
