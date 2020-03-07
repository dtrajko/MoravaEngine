#pragma once

#include "Shader.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"


class ShaderMain : public Shader
{
public:
	ShaderMain();

	GLint GetUniformLocationEyePosition();

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

	// Directional light shadow map
	void SetDirectionalShadowMap(GLuint textureUnit);
	void SetDirectionalLightTransform(glm::mat4* transform);

	void SetClipPlane(glm::vec4 clipPlane);

	~ShaderMain();

private:
	virtual void GetUniformLocations() override;

private:
	int pointLightCount = 0;
	int spotLightCount = 0;

	GLint uniformEyePosition = -1;
	GLint uniformDirectionalShadowMap = -1;
	GLint uniformDirectionalLightTransform = -1;

	// Setting the gl_ClipDistance
	GLuint uniformClipPlane = -1;

	struct
	{
		GLint uniformColor = -1;
		GLint uniformAmbientIntensity = -1;
		GLint uniformDiffuseIntensity = -1;
		GLint uniformDirection = -1;
	} uniformDirectionalLight;

	GLint uniformPointLightCount = -1;

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

	GLint uniformSpotLightCount = -1;

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

	GLint uniformMaterialSpecularIntensity = -1;
	GLint uniformMaterialShininess = -1;

};
