#include "ShaderWater.h"


ShaderWater::ShaderWater() : Shader()
{
}

GLint ShaderWater::GetUniformLocationReflectionTexture()
{
	return uniformReflectionTexture;
}

GLint ShaderWater::GetUniformLocationRefractionTexture()
{
	return uniformRefractionTexture;
}

GLint ShaderWater::GetUniformLocationDuDvMap()
{
	return uniformDuDvMap;
}

GLint ShaderWater::GetUniformLocationMoveFactor()
{
	return uniformMoveFactor;
}

GLint ShaderWater::GetUniformLocationCameraPosition()
{
	return uniformCameraPosition;
}

GLint ShaderWater::GetUniformLocationLightColor()
{
	return uniformLightColor;
}

void ShaderWater::SetWater(unsigned int txUnitReflection, unsigned int txUnitRefraction,
	unsigned int txUnitDuDv, unsigned int txUnitDepth)
{
	glUniform1i(uniformReflectionTexture, txUnitReflection);
	glUniform1i(uniformRefractionTexture, txUnitRefraction);
	glUniform1i(uniformDuDvMap, txUnitDuDv);
	glUniform1i(uniformDepthMap, txUnitDepth);
}

void ShaderWater::SetMoveFactor(float moveFactor)
{
	glUniform1f(uniformMoveFactor, moveFactor);
}

void ShaderWater::SetCameraPosition(glm::vec3 cameraPosition)
{
	glUniform3f(uniformCameraPosition, cameraPosition.x, cameraPosition.y, cameraPosition.z);
}

void ShaderWater::SetLightColor(glm::vec3 lightColor)
{
	glUniform3f(uniformLightColor, lightColor.r, lightColor.g, lightColor.b);
}

void ShaderWater::SetDuDvMap(GLuint textureUnit)
{
	glUniform1i(uniformDuDvMap, textureUnit);
}

void ShaderWater::GetUniformLocations()
{
	printf("ShaderWater::GetUniformLocations\n");

	Shader::GetUniformLocations();

	// Water shader sampler2D uniforms
	uniformReflectionTexture = glGetUniformLocation(programID, "reflectionTexture");
	uniformRefractionTexture = glGetUniformLocation(programID, "refractionTexture");
	uniformDuDvMap = glGetUniformLocation(programID, "dudvMap");
	uniformNormalMap = glGetUniformLocation(programID, "normalMap");
	uniformDepthMap = glGetUniformLocation(programID, "depthMap");
	uniformLightColor = glGetUniformLocation(programID, "lightColor");
	uniformMoveFactor = glGetUniformLocation(programID, "moveFactor");
	uniformLightPosition = glGetUniformLocation(programID, "lightPosition");
	uniformCameraPosition = glGetUniformLocation(programID, "cameraPosition");
}

ShaderWater::~ShaderWater()
{
}
