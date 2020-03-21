#include "ShaderMain.h"


ShaderMain::ShaderMain() : Shader()
{
	pointLightCount = 0;
	spotLightCount = 0;
}

GLint ShaderMain::GetUniformLocationAmbientColor()
{
	return uniformDirectionalLight.uniformColor;
}

GLint ShaderMain::GetUniformLocationAmbientIntensity()
{
	return uniformDirectionalLight.uniformAmbientIntensity;
}

GLint ShaderMain::GetUniformLocationDiffuseIntensity()
{
	return uniformDirectionalLight.uniformDiffuseIntensity;
}

GLint ShaderMain::GetUniformLocationDirectionalLightDirection()
{
	return uniformDirectionalLight.uniformDirection;
}

GLint ShaderMain::GetUniformLocationMaterialSpecularIntensity()
{
	return uniformMaterial.uniformSpecularIntensity;
}

GLint ShaderMain::GetUniformLocationMaterialShininess()
{
	return uniformMaterial.uniformShininess;
}

void ShaderMain::SetDirectionalLight(DirectionalLight* directionalLight)
{
	directionalLight->UseLight(
		uniformDirectionalLight.uniformColor,
		uniformDirectionalLight.uniformAmbientIntensity,
		uniformDirectionalLight.uniformDiffuseIntensity,
		uniformDirectionalLight.uniformDirection);
}

void ShaderMain::SetPointLights(PointLight* pointLights, unsigned int lightCount, unsigned int textureUnit, unsigned int offset)
{
	if (lightCount > MAX_POINT_LIGHTS) lightCount = MAX_POINT_LIGHTS;

	setInt("pointLightCount", lightCount);

	for (unsigned int i = 0; i < lightCount; i++)
	{
		pointLights[i].UseLight(
			uniformPointLight[i].uniformColor,
			uniformPointLight[i].uniformAmbientIntensity,
			uniformPointLight[i].uniformDiffuseIntensity,
			uniformPointLight[i].uniformPosition,
			uniformPointLight[i].uniformConstant,
			uniformPointLight[i].uniformLinear,
			uniformPointLight[i].uniformExponent);

		pointLights[i].GetShadowMap()->Bind(textureUnit + offset + i);
		glUniform1i(uniformOmniShadowMap[offset + i].shadowMap, textureUnit + offset + i);
		glUniform1f(uniformOmniShadowMap[offset + i].farPlane, pointLights[i].GetFarPlane());
	}
}

void ShaderMain::SetSpotLights(SpotLight* spotLights, unsigned int lightCount, unsigned int textureUnit, unsigned int offset)
{
	if (lightCount > MAX_SPOT_LIGHTS) lightCount = MAX_SPOT_LIGHTS;

	setInt("spotLightCount", lightCount);

	for (unsigned int i = 0; i < lightCount; i++)
	{
		spotLights[i].UseLight(
			uniformSpotLight[i].uniformColor,
			uniformSpotLight[i].uniformAmbientIntensity,
			uniformSpotLight[i].uniformDiffuseIntensity,
			uniformSpotLight[i].uniformPosition,
			uniformSpotLight[i].uniformDirection,
			uniformSpotLight[i].uniformConstant,
			uniformSpotLight[i].uniformLinear,
			uniformSpotLight[i].uniformExponent,
			uniformSpotLight[i].uniformEdge);

		spotLights[i].GetShadowMap()->Bind(textureUnit + offset + i);
		glUniform1i(uniformOmniShadowMap[offset + i].shadowMap, textureUnit + offset + i);
		glUniform1f(uniformOmniShadowMap[offset + i].farPlane, spotLights[i].GetFarPlane());
	}
}

ShaderMain::~ShaderMain()
{
}

void ShaderMain::GetUniformLocations()
{
	printf("ShaderMain::GetUniformLocations\n");

	Shader::GetUniformLocations();

	uniformDirectionalLight.uniformColor = glGetUniformLocation(programID, "directionalLight.base.color");
	uniformDirectionalLight.uniformAmbientIntensity = glGetUniformLocation(programID, "directionalLight.base.ambientIntensity");
	uniformDirectionalLight.uniformDiffuseIntensity = glGetUniformLocation(programID, "directionalLight.base.diffuseIntensity");
	uniformDirectionalLight.uniformDirection = glGetUniformLocation(programID, "directionalLight.direction");

	uniformMaterial.uniformSpecularIntensity = glGetUniformLocation(programID, "material.specularIntensity");
	uniformMaterial.uniformShininess = glGetUniformLocation(programID, "material.shininess");

	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.color", i);
		uniformPointLight[i].uniformColor = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.ambientIntensity", i);
		uniformPointLight[i].uniformAmbientIntensity = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.diffuseIntensity", i);
		uniformPointLight[i].uniformDiffuseIntensity = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].position", i);
		uniformPointLight[i].uniformPosition = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].constant", i);
		uniformPointLight[i].uniformConstant = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].linear", i);
		uniformPointLight[i].uniformLinear = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].exponent", i);
		uniformPointLight[i].uniformExponent = glGetUniformLocation(programID, locBuff);
	}

	for (int i = 0; i < MAX_SPOT_LIGHTS; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.color", i);
		uniformSpotLight[i].uniformColor = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.ambientIntensity", i);
		uniformSpotLight[i].uniformAmbientIntensity = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.diffuseIntensity", i);
		uniformSpotLight[i].uniformDiffuseIntensity = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.position", i);
		uniformSpotLight[i].uniformPosition = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].direction", i);
		uniformSpotLight[i].uniformDirection = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.constant", i);
		uniformSpotLight[i].uniformConstant = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.linear", i);
		uniformSpotLight[i].uniformLinear = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.exponent", i);
		uniformSpotLight[i].uniformExponent = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].edge", i);
		uniformSpotLight[i].uniformEdge = glGetUniformLocation(programID, locBuff);
	}

	for (unsigned int i = 0; i < MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", i);
		uniformOmniShadowMap[i].shadowMap = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", i);
		uniformOmniShadowMap[i].farPlane = glGetUniformLocation(programID, locBuff);
	}
}
