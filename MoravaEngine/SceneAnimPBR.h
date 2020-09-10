#pragma once

#include "Scene.h"
#include "Hazel/Renderer/MeshAnimPBR.h"
#include "TextureCubemapLite.h"
#include "CubeSkybox.h"
#include "MaterialWorkflowPBR.h"


const int MAX_LIGHTS = 4 + 4; // (4 x point lights) + (4 x spot lights)

class SceneAnimPBR : public Scene
{

public:
	SceneAnimPBR();
	virtual ~SceneAnimPBR() override;

	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;
	void SetupUniforms();

	void EditTransform(const float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition);

private:
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMaterials() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;

	void SetupShaders(); // Usually in Renderer* classes

	std::pair<TextureCubemapLite*, TextureCubemapLite*> CreateEnvironmentMap(const std::string& filepath);

	// ImGuizmo
	float GetSnapValue();

private:
	glm::mat4 m_CubeTransform;
	int m_GizmoType;

	Shader* m_ShaderMain;
	Shader* m_ShaderBackground;
	// Shader* m_ShaderHazelAnimPBR;
	Shader* m_ShaderHybridAnimPBR;
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

	glm::vec3 m_LightPosition;
	glm::vec3 m_LightColor;

};
