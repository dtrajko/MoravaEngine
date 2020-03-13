#include "ShaderSkyboxJoey.h"



ShaderSkyboxJoey::ShaderSkyboxJoey() : Shader()
{
}

GLint ShaderSkyboxJoey::GetUniformLocationEnvironmentMap()
{
	return uniformEnvironmentMap;
}

void ShaderSkyboxJoey::SetEnvironmentMap(int environmentMap)
{
	glUniform1i(uniformEnvironmentMap, environmentMap);
}

void ShaderSkyboxJoey::GetUniformLocations()
{
	printf("ShaderSkyboxJoey::GetUniformLocations\n");

	Shader::GetUniformLocations();

	uniformEnvironmentMap = glGetUniformLocation(programID, "environmentMap");
}

ShaderSkyboxJoey::~ShaderSkyboxJoey()
{
}
