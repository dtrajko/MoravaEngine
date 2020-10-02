#pragma once

#include "Shader.h"
#include "TextureCubemap.h"
#include "Material.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Mesh.h"


struct Environment
{
	TextureCubemap* RadianceMap;
	TextureCubemap* IrradianceMap;
};

class EnvironmentMap
{
	struct Light;
	struct AlbedoInput;
	struct NormalInput;
	struct MetalnessInput;
	struct RoughnessInput;

public:
	EnvironmentMap() = default;
	EnvironmentMap(const std::string& filepath);
	~EnvironmentMap();

	void Update();
	Environment Load(const std::string& filepath);
	void SetEnvironment(Environment environment);

	// Getters
	inline Shader* GetPBRShader() { return m_ShaderHazelAnimPBR; };
	inline std::map<std::string, unsigned int>* GetSamplerSlots() { return m_SamplerSlots; }
	float& GetSkyboxLod() { return m_SkyboxLod; }
	Light& GetLight() { return m_Light; }
	inline bool& GetRadiancePrefilter() { return m_RadiancePrefilter; }
	float& GetEnvMapRotation() { return m_EnvMapRotation; }
	Hazel::HazelTexture2D* GetCheckerboardTexture() { return m_CheckerboardTexture; }
	AlbedoInput& GetAlbedoInput() { return m_AlbedoInput; };
	NormalInput& GetNormalInput() { return m_NormalInput; };
	MetalnessInput& GetMetalnessInput() { return m_MetalnessInput; };
	RoughnessInput& GetRoughnessInput() { return m_RoughnessInput; };

private:
	void SetupShaders();
	void UpdateUniforms();
	std::pair<TextureCubemap*, TextureCubemap*> CreateEnvironmentMap(const std::string& filepath);
	void SetSkybox(TextureCubemap* skybox);

private:
	Shader* m_ShaderEquirectangularConversion;
	Shader* m_ShaderEnvFiltering;
	Shader* m_ShaderEnvIrradiance;
	Shader* m_ShaderSkybox;
	Shader* m_ShaderHazelAnimPBR;
	Shader* m_ShaderComposite;

	Environment m_Environment;
	TextureCubemap* m_SkyboxTexture;
	Material* m_SkyboxMaterial;

	std::map<std::string, unsigned int>* m_SamplerSlots;

	/**** BEGIN properties Scene ****/
	struct Light
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;

		float Multiplier = 1.0f;
	};
	Light m_Light;

	float m_SkyboxLod = 1.0f;
	/**** END properties Scene ****/

	/**** BEGIN properties SceneRenderer ****/
	Hazel::HazelTexture2D* m_BRDF_LUT;

	Hazel::RenderPass* m_RenderPassGeo;
	Hazel::RenderPass* m_RenderPassComposite;

	struct DrawCommand
	{
		Mesh* Mesh;
		Material* Material;
		glm::mat4 Transform;
	};
	std::vector<DrawCommand> DrawList;
	/**** END properties SceneRenderer ****/

	/**** BEGIN properties EditorLayer ****/
	struct AlbedoInput
	{
		glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
		Hazel::HazelTexture2D* TextureMap;
		bool SRGB = true;
		bool UseTexture = false;
	};
	AlbedoInput m_AlbedoInput;

	struct NormalInput
	{
		Hazel::HazelTexture2D* TextureMap;
		bool UseTexture = false;
	};
	NormalInput m_NormalInput;

	struct MetalnessInput
	{
		float Value = 1.0f;
		Hazel::HazelTexture2D* TextureMap;
		bool UseTexture = false;
	};
	MetalnessInput m_MetalnessInput;

	struct RoughnessInput
	{
		float Value = 0.2f;
		Hazel::HazelTexture2D* TextureMap;
		bool UseTexture = false;
	};
	RoughnessInput m_RoughnessInput;

	// PBR params
	bool m_RadiancePrefilter = false;

	float m_EnvMapRotation = 0.0f;

	// Editor resources
	Hazel::HazelTexture2D* m_CheckerboardTexture;

	glm::vec2 m_ViewportBounds[2];
	int m_GizmoType = -1; // -1 = no gizmo
	/**** END properties EditorLayer ****/

};
