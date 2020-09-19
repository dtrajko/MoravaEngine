#pragma once

#include "Scene.h"
#include "Hazel/Renderer/MeshAnimPBR.h"
#include "TextureCubemapLite.h"
#include "CubeSkybox.h"
#include "MaterialWorkflowPBR.h"
#include "AABB.h"
#include "Framebuffer.h"


const int MAX_LIGHTS = 4 + 4; // (4 x point lights) + (4 x spot lights)

class SceneAnimPBR : public Scene
{

public:
	SceneAnimPBR();
	virtual ~SceneAnimPBR() override;

	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	void UpdateImGuizmo(Window& mainWindow);
	virtual void ShowExampleAppDockSpace(bool* p_open, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;
	void SetupUniforms();

private:
	virtual void SetLightManager() override;
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMaterials() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;

	void SetupShaders(); // Usually in Renderer* classes

	void SetupRenderFramebuffer();
	void ResizeViewport(glm::vec2 viewportPanelSize);

private:
	Shader* m_ShaderMain;
	Shader* m_ShaderBackground;
	Shader* m_ShaderHybridAnimPBR;
	Shader* m_ShaderHDR;
	Shader* m_ShaderEquirectangularConversion;
	Shader* m_ShaderEnvFiltering;
	Shader* m_ShaderEnvIrradiance;
	Shader* m_ShaderBasic;

	Hazel::MeshAnimPBR* m_MeshAnimPBR_M1911;
	Hazel::MeshAnimPBR* m_MeshAnimPBR_BobLamp;
	Hazel::MeshAnimPBR* m_MeshAnimPBR_AnimBoy;

	Material* m_BaseMaterial_M1911;
	Material* m_BaseMaterial_BobLamp;
	Material* m_BaseMaterial_AnimBoy;

	glm::vec3 m_Position_M1911;
	glm::vec3 m_Position_BobLamp;
	glm::vec3 m_Position_AnimBoy;
	glm::vec3 m_Position_Cube;

	glm::vec3 m_Scale_M1911;
	glm::vec3 m_Scale_BobLamp;
	glm::vec3 m_Scale_AnimBoy;
	glm::vec3 m_Scale_Cube;

	AABB* m_AABB_M1911;
	AABB* m_AABB_BobLamp;
	AABB* m_AABB_AnimBoy;
	AABB* m_AABB_Cube;

	glm::mat4 m_Transform_M1911;
	glm::mat4 m_Transform_BobLamp;
	glm::mat4 m_Transform_AnimBoy;
	glm::mat4 m_Transform_Cube;

	glm::mat4 m_Transform_Gizmo;

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

	float m_SkyboxLOD;

	int m_GizmoType = -1; // -1 = no gizmo

	// viewport
	bool m_IsViewportEnabled;
	bool m_ViewportFocused;
	bool m_ViewportHovered;
	glm::vec2 m_ViewportSize;
	Framebuffer* m_RenderFramebuffer;
	float m_CurrentTimestamp;
	EventCooldown m_ResizeViewport;

	bool m_VisibleAABBs;

};
