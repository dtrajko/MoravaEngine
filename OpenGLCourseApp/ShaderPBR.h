#pragma once

#include "Shader.h"


class ShaderPBR : public Shader
{
public:
	ShaderPBR();

	GLint GetUniformLocationAlbedo();
	GLint GetUniformLocationMetallic();
	GLint GetUniformLocationRoughness();
	GLint GetUniformLocationAmbientOcclusion();

	GLint GetUniformLocationAlbedoMap();
	GLint GetUniformLocationNormalMap();
	GLint GetUniformLocationMetallicMap();
	GLint GetUniformLocationRoughnessMap();
	GLint GetUniformLocationAmbientOcclusionMap();

	GLint GetUniformLocationCameraPosition();
	GLint GetUniformLocationAmbientIntensity();

	void SetAlbedo(glm::vec3 albedo);
	void SetMetallic(float metallic);
	void SetRoughness(float roughness);
	void SetAmbientOcclusion(float ao);

	void SetAlbedoMap(unsigned int textureUnit);
	void SetNormalMap(unsigned int textureUnit);
	void SetMetallicMap(unsigned int textureUnit);
	void SetRoughnessMap(unsigned int textureUnit);
	void SetAmbientOcclusionMap(unsigned int textureUnit);

	void SetCameraPosition(glm::vec3 cameraPosition);
	void SetAmbientIntensity(float ambientIntensity);

	void SetLightPositions(glm::vec3* lightPositions, unsigned int lightCount);
	void SetLightColors(glm::vec3* lightColors, unsigned int lightCount);

	~ShaderPBR();

private:
	virtual void GetUniformLocations() override;

private:
	static const unsigned int lightCount = 4;

	GLint uniformAlbedo = -1;
	GLint uniformMetallic = -1;
	GLint uniformRoughness = -1;
	GLint uniformAmbientOcclusion = -1;

	// sampler2D uniform locations
	GLint uniformAlbedoMap = -1;
	GLint uniformNormalMap = -1;
	GLint uniformMetallicMap = -1;
	GLint uniformRoughnessMap = -1;
	GLint uniformAmbientOcclusionMap = -1;

	GLint uniformCameraPosition = -1;
	GLint uniformAmbientIntensity = -1;

	// lights
	GLint uniformLightPositions[lightCount];
	GLint uniformLightColors[lightCount];

};
