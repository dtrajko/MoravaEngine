#pragma once

#include "Shader.h"
#include "TextureCubemap.h"
#include "Material.h"


struct Environment
{
	TextureCubemap* RadianceMap;
	TextureCubemap* IrradianceMap;
};

class EnvironmentMap
{

public:
	EnvironmentMap() = default;
	EnvironmentMap(const std::string& filepath);
	~EnvironmentMap();

private:
	void SetupShaders();
	Environment Load(const std::string& filepath);
	std::pair<TextureCubemap*, TextureCubemap*> CreateEnvironmentMap(const std::string& filepath);
	void SetEnvironment(Environment* environment);
	void SetSkybox(TextureCubemap* skybox);

private:
	Shader* m_ShaderEquirectangularConversion;
	Shader* m_ShaderEnvFiltering;
	Shader* m_ShaderEnvIrradiance;
	Shader* m_ShaderSkybox;
	Shader* m_ShaderHazelAnimPBR;

	Environment* m_Environment;
	TextureCubemap* m_SkyboxTexture;
	Material* m_SkyboxMaterial;

	/**** BEGIN properties EditorLayer ****/
	struct AlbedoInput
	{
		glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
		Texture* TextureMap;
		bool SRGB = true;
		bool UseTexture = false;
	};
	AlbedoInput m_AlbedoInput;

	struct NormalInput
	{
		Texture* TextureMap;
		bool UseTexture = false;
	};
	NormalInput m_NormalInput;

	struct MetalnessInput
	{
		float Value = 1.0f;
		Texture* TextureMap;
		bool UseTexture = false;
	};
	MetalnessInput m_MetalnessInput;

	struct RoughnessInput
	{
		float Value = 0.2f;
		Texture* TextureMap;
		bool UseTexture = false;
	};
	RoughnessInput m_RoughnessInput;

	// PBR params
	bool m_RadiancePrefilter = false;

	float m_EnvMapRotation = 0.0f;

	// Editor resources
	Texture* m_CheckerboardTex;

	glm::vec2 m_ViewportBounds[2];
	int m_GizmoType = -1; // -1 = no gizmo
	/**** END properties EditorLayer ****/

	/**** BEGIN properties Scene ****/
	struct Light
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;

		float Multiplier = 1.0f;
	};
	Light m_Lights;
	/**** END properties Scene ****/

};
