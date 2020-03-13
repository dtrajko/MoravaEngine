#include "ShaderCubemap.h"



ShaderCubemap::ShaderCubemap() : Shader()
{
}

GLint ShaderCubemap::GetUniformLocationEquirectangularMap()
{
	return uniformEquirectangularMap;
}

void ShaderCubemap::SetEquirectangularMap(int equirectangularMap)
{
	glUniform1i(uniformEquirectangularMap, equirectangularMap);
}

void ShaderCubemap::GetUniformLocations()
{
	printf("ShaderCubemap::GetUniformLocations\n");

	Shader::GetUniformLocations();

	uniformEquirectangularMap = glGetUniformLocation(programID, "equirectangularMap");
}

ShaderCubemap::~ShaderCubemap()
{
}
