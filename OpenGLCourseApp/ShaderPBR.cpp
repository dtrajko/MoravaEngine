#include "ShaderPBR.h"


const unsigned int ShaderPBR::lightCount;


ShaderPBR::ShaderPBR() : Shader()
{
	for (int i = 0; i < lightCount; i++)
	{
		uniformLightPositions[i] = -1;
	}
	for (int i = 0; i < lightCount; i++)
	{
		uniformLightColors[i] = -1;
	}
}

GLint ShaderPBR::GetUniformLocationAlbedo()
{
	return uniformAlbedo;
}

GLint ShaderPBR::GetUniformLocationMetallic()
{
	return uniformMetallic;
}

GLint ShaderPBR::GetUniformLocationRoughness()
{
	return uniformRoughness;
}

GLint ShaderPBR::GetUniformLocationAmbientOcclusion()
{
	return uniformAmbientOcclusion;
}

GLint ShaderPBR::GetUniformLocationAlbedoMap()
{
	return uniformAlbedoMap;
}

GLint ShaderPBR::GetUniformLocationNormalMap()
{
	return uniformNormalMap;
}

GLint ShaderPBR::GetUniformLocationMetallicMap()
{
	return uniformMetallicMap;
}

GLint ShaderPBR::GetUniformLocationRoughnessMap()
{
	return uniformRoughnessMap;
}

GLint ShaderPBR::GetUniformLocationAmbientOcclusionMap()
{
	return uniformAmbientOcclusionMap;
}

GLint ShaderPBR::GetUniformLocationCameraPosition()
{
	return uniformCameraPosition;
}

GLint ShaderPBR::GetUniformLocationAmbientIntensity()
{
	return uniformAmbientIntensity;
}

void ShaderPBR::SetAlbedo(glm::vec3 albedo)
{
	glUniform3f(uniformAlbedo, albedo.r, albedo.g, albedo.b);
}

void ShaderPBR::SetMetallic(float metallic)
{
	glUniform1f(uniformMetallic, metallic);
}

void ShaderPBR::SetRoughness(float roughness)
{
	glUniform1f(uniformRoughness, roughness);
}

void ShaderPBR::SetAmbientOcclusion(float ao)
{
	glUniform1f(uniformAmbientOcclusion, ao);
}

void ShaderPBR::SetAlbedoMap(unsigned int textureUnit)
{
	glUniform1i(uniformAlbedoMap, textureUnit);
}

void ShaderPBR::SetNormalMap(unsigned int textureUnit)
{
	glUniform1i(uniformNormalMap, textureUnit);
}

void ShaderPBR::SetMetallicMap(unsigned int textureUnit)
{
	glUniform1i(uniformMetallicMap, textureUnit);
}

void ShaderPBR::SetRoughnessMap(unsigned int textureUnit)
{
	glUniform1i(uniformRoughnessMap, textureUnit);
}

void ShaderPBR::SetAmbientOcclusionMap(unsigned int textureUnit)
{
	glUniform1i(uniformAmbientOcclusionMap, textureUnit);
}

void ShaderPBR::SetCameraPosition(glm::vec3 cameraPosition)
{
	glUniform3f(uniformCameraPosition, cameraPosition.x, cameraPosition.y, cameraPosition.z);
}

void ShaderPBR::SetAmbientIntensity(float ambientIntensity)
{
	glUniform1f(uniformAmbientIntensity, ambientIntensity);
}

void ShaderPBR::SetLightPositions(glm::vec3* lightPositions, unsigned int lightCount)
{
	for (unsigned int i = 0; i < lightCount; ++i)
	{
		glUniform3f(uniformLightPositions[i], lightPositions[i].x, lightPositions[i].y, lightPositions[i].z);
	}
}

void ShaderPBR::SetLightColors(glm::vec3* lightColors, unsigned int lightCount)
{
	for (unsigned int i = 0; i < lightCount; ++i)
	{
		glUniform3f(uniformLightColors[i], lightColors[i].x, lightColors[i].y, lightColors[i].z);
	}
}

void ShaderPBR::GetUniformLocations()
{
	printf("ShaderWater::GetUniformLocations\n");

	Shader::GetUniformLocations();

	uniformAlbedo = glGetUniformLocation(programID, "albedo");
	uniformMetallic = glGetUniformLocation(programID, "metallic");
	uniformRoughness = glGetUniformLocation(programID, "roughness");
	uniformAmbientOcclusion = glGetUniformLocation(programID, "ao");

	uniformAlbedoMap = glGetUniformLocation(programID, "albedoMap");
	uniformNormalMap = glGetUniformLocation(programID, "normalMap");
	uniformMetallicMap = glGetUniformLocation(programID, "metallicMap");
	uniformRoughnessMap = glGetUniformLocation(programID, "roughnessMap");
	uniformAmbientOcclusionMap = glGetUniformLocation(programID, "aoMap");

	uniformCameraPosition = glGetUniformLocation(programID, "camPos");
	uniformAmbientIntensity = glGetUniformLocation(programID, "ambientIntensity");

	for (int i = 0; i < lightCount; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "lightPositions[%d]", i);
		uniformLightPositions[i] = glGetUniformLocation(programID, locBuff);
	}

	for (int i = 0; i < lightCount; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "lightColors[%d]", i);
		uniformLightColors[i] = glGetUniformLocation(programID, locBuff);
	}
}

ShaderPBR::~ShaderPBR()
{
}
