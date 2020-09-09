#pragma once

#include "Scene.h"
#include "Hazel/Renderer/MeshAnimPBR.h"
#include "TextureCubemapLite.h"
#include "CubeSkybox.h"
#include "MaterialWorkflowPBR.h"


class SceneAnimPBR : public Scene
{

public:
	SceneAnimPBR();
	virtual ~SceneAnimPBR() override;

	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	void SetupUniforms();

	void EditTransform(const float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition);

private:
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;

	void SetupShaders(); // Usually in Renderer* classes

	std::pair<TextureCubemapLite*, TextureCubemapLite*> CreateEnvironmentMap(const std::string& filepath);

	// ImGuizmo
	float GetSnapValue();

private:
	struct Light
	{
		glm::vec3 Direction;
		float Radiance;
		float Multiplier;
	} m_Light;

	glm::vec3 m_AlbedoColor;
	float m_Roughness;

	glm::mat4 m_CubeTransform;
	int m_GizmoType;

	Shader* m_ShaderMain;
	Shader* m_ShaderBackground;
	Shader* m_ShaderHazelAnimPBR;
	Shader* m_ShaderHDR;
	Shader* m_ShaderEquirectangularConversion;
	Shader* m_ShaderEnvFiltering;
	Shader* m_ShaderEnvIrradiance;

	Hazel::MeshAnimPBR* m_MeshAnimPBRM1911;
	Hazel::MeshAnimPBR* m_MeshAnimPBRBob;
	Hazel::MeshAnimPBR* m_MeshAnimPBRBoy;

	Material* m_BaseMaterialM1911;
	Material* m_BaseMaterialBob;
	Material* m_BaseMaterialBoy;

	glm::mat4 m_Transform_M1911;
	glm::mat4 m_Transform_BobLamp;
	glm::mat4 m_Transform_Boy;

	bool m_RadiancePrefilter  = true;
	bool m_AlbedoTexToggle    = true;
	bool m_NormalTexToggle    = true;
	bool m_MetalnessTexToggle = true;
	bool m_RoughnessTexToggle = true;

	float m_EnvMapRotation = 0.0f;

	std::map<std::string, unsigned int> m_SamplerSlots;

	MaterialWorkflowPBR* m_MaterialWorkflowPBR;

	int m_HDRI_Edit;
	int m_HDRI_Edit_Prev;

	// TextureCubemapLite* m_EnvUnfiltered;
	// Texture* m_EnvEquirect;
	// TextureCubemapLite* m_EnvFiltered;
	// TextureCubemapLite* m_IrradianceMap;
	// Texture* m_BRDF_LUT;
	// std::pair<TextureCubemapLite*, TextureCubemapLite*> m_TextureCubemaps;

};
