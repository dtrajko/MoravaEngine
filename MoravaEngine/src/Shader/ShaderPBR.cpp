#include "Shader/ShaderPBR.h"


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
