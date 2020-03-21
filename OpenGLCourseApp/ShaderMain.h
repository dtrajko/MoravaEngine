#pragma once

#include "Shader.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"


class ShaderMain : public Shader
{
public:
	ShaderMain();

	// Directional Light
	GLint GetUniformLocationAmbientColor();
	GLint GetUniformLocationAmbientIntensity();
	GLint GetUniformLocationDiffuseIntensity();
	GLint GetUniformLocationDirectionalLightDirection();

	GLint GetUniformLocationMaterialSpecularIntensity();
	GLint GetUniformLocationMaterialShininess();

	void SetDirectionalLight(DirectionalLight* directionalLight);
	void SetPointLights(PointLight* pointLights, unsigned int lightCount, unsigned int textureUnit, unsigned int offset);
	void SetSpotLights(SpotLight* spotLights, unsigned int lightCount, unsigned int textureUnit, unsigned int offset);

	~ShaderMain();

private:
	virtual void GetUniformLocations() override;

private:
	int pointLightCount = 0;
	int spotLightCount = 0;

	struct
	{
		GLint uniformColor = -1;
		GLint uniformAmbientIntensity = -1;
		GLint uniformDiffuseIntensity = -1;
		GLint uniformDirection = -1;
	} uniformDirectionalLight;

	struct
	{
		GLint uniformColor = -1;
		GLint uniformAmbientIntensity = -1;
		GLint uniformDiffuseIntensity = -1;
		GLint uniformPosition = -1;
		GLint uniformConstant = -1;
		GLint uniformLinear = -1;
		GLint uniformExponent = -1;
	} uniformPointLight[MAX_POINT_LIGHTS];

	struct
	{
		GLint uniformColor = -1;
		GLint uniformAmbientIntensity = -1;
		GLint uniformDiffuseIntensity = -1;
		GLint uniformPosition = -1;
		GLint uniformDirection = -1;
		GLint uniformConstant = -1;
		GLint uniformLinear = -1;
		GLint uniformExponent = -1;
		GLint uniformEdge = -1;
	} uniformSpotLight[MAX_SPOT_LIGHTS];

	struct
	{
		GLint shadowMap = -1;
		GLint farPlane = -1;
	} uniformOmniShadowMap[MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS];

	struct
	{
		GLint uniformSpecularIntensity = -1;
		GLint uniformShininess = -1;
	} uniformMaterial;

};
