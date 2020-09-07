#pragma once

#include "Scene.h"
#include "Hazel/Renderer/MeshAnimPBR.h"
#include "TextureCubemapLite.h"


class SceneImGuizmo : public Scene
{

public:
	SceneImGuizmo();
	virtual ~SceneImGuizmo() override;

	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;

	void EditTransform(const float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition);

private:
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;

	std::pair<TextureCubemapLite*, TextureCubemapLite*> CreateEnvironmentMap(const std::string& filepath);

	// ImGuizmo
	float GetSnapValue();

	glm::mat4 m_CubeTransform;
	int m_GizmoType;

	Hazel::MeshAnimPBR* m_MeshAnimPBR;

	Shader* m_ShaderHazelAnimPBR;
	Shader* m_ShaderHDR;
	Shader* m_ShaderEquirectangularConversion;
	Shader* m_ShaderEnvFiltering;
	Shader* m_ShaderEnvIrradiance;

	Material* m_BaseMaterial;

	Texture* m_HDR;
	std::pair<TextureCubemapLite*, TextureCubemapLite*> m_TextureCubemaps;

	bool m_AlbedoTexToggle    = true;
	bool m_NormalTexToggle    = true;
	bool m_MetalnessTexToggle = true;
	bool m_RoughnessTexToggle = true;

	float m_EnvMapRotation = 0.0f;
	glm::mat4 m_M1911_Transform;

	std::map<std::string, unsigned int> m_SamplerSlots;

	TextureCubemapLite* m_EnvUnfiltered;
	Texture* m_EnvEquirect;
	TextureCubemapLite* m_EnvFiltered;
	TextureCubemapLite* m_IrradianceMap;

	Texture* m_BRDF_LUT;

};
