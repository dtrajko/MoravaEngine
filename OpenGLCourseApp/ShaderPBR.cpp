#include "ShaderPBR.h"


ShaderPBR::ShaderPBR() : Shader()
{
}

GLint ShaderPBR::GetUniformLocationReflectionTexture()
{
	return uniformReflectionTexture;
}

GLint ShaderPBR::GetUniformLocationRefractionTexture()
{
	return uniformRefractionTexture;
}

GLint ShaderPBR::GetUniformLocationDuDvMap()
{
	return uniformDuDvMap;
}

GLint ShaderPBR::GetUniformLocationMoveFactor()
{
	return uniformMoveFactor;
}

GLint ShaderPBR::GetUniformLocationCameraPosition()
{
	return uniformCameraPosition;
}

GLint ShaderPBR::GetUniformLocationLightColor()
{
	return uniformLightColor;
}

void ShaderPBR::SetMoveFactor(float moveFactor)
{
	glUniform1f(uniformMoveFactor, moveFactor);
}

void ShaderPBR::SetCameraPosition(glm::vec3 cameraPosition)
{
	glUniform3f(uniformCameraPosition, cameraPosition.x, cameraPosition.y, cameraPosition.z);
}

void ShaderPBR::SetLightColor(glm::vec3 lightColor)
{
	glUniform3f(uniformLightColor, lightColor.r, lightColor.g, lightColor.b);
}

void ShaderPBR::SetDuDvMap(GLuint textureUnit)
{
	glUniform1i(uniformDuDvMap, textureUnit);
}

void ShaderPBR::GetUniformLocations()
{
	printf("ShaderWater::GetUniformLocations\n");

	Shader::GetUniformLocations();

	// Water shader sampler2D uniforms
	uniformReflectionTexture = glGetUniformLocation(programID, "reflectionTexture");
	// ...
}

ShaderPBR::~ShaderPBR()
{
}
