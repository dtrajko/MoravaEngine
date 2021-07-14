#pragma once

#include "Scene/Scene.h"

#include "Core/Raycast.h"
#include "Editor/Gizmo.h"
#include "Editor/SceneObjectParticleSystem.h"
#include "Framebuffer/MoravaFramebuffer.h"
#include "Material/MaterialWorkflowPBR.h"
#include "Mesh/Grid.h"
#include "Mesh/Quad.h"
#include "Mesh/SkinnedMesh.h"
#include "PostProcessing/BlurEffect.h"
#include "Texture/TextureCubemapFaces.h"

#include <future>
#include <set>


class SceneEditor : public Scene
{
public:
	SceneEditor();
    virtual ~SceneEditor() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, MoravaShader*> shaders, std::map<std::string, int> uniforms) override;
    inline MoravaFramebuffer* GetRenderFramebuffer() { return m_RenderFramebuffer; };

private:
	virtual void SetupTextures()   override;
	virtual void SetupMaterials()  override;
	virtual void SetupMeshes()     override;
	virtual void SetupModels()     override;
	virtual void SetSkybox()       override;
	virtual void SetLightManager() override;
    void SetupRenderFramebuffer();
	void AddLightsToSceneObjects();

	void RenderLightSources(MoravaShader* shader);
	void RenderSkybox(MoravaShader* shader);
	void RenderLineElements(MoravaShader* shader, glm::mat4 projectionMatrix);
	void RenderFramebufferTextures(MoravaShader* shader);
	void RenderGlassObjects(MoravaShader* shader);
	void SetGeometry();
	void CleanupGeometry();
	inline Raycast* GetRaycast() const { return m_Raycast; };
	SceneObject* CreateNewSceneObject();
	void AddSceneObject();
	void CopySceneObject(Window* mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	void DeleteSceneObject(Window* mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	void SelectNextFromMultipleObjects(std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	bool IsObjectSelected(unsigned int objectIndex);
	void SaveScene();
	void LoadScene();
	void ResetScene();
	Mesh* CreateNewMesh(int meshTypeID, glm::vec3 scale, std::string* name);
	Model* AddNewModel(int modelID, glm::vec3 scale);
	SceneObjectParticleSystem* AddNewSceneObjectParticleSystem(int objectTypeID, glm::vec3 scale);
	void SetUniformsShaderEditorPBR(MoravaShader* shaderEditorPBR, Texture* texture, Hazel::Ref<Material> material, SceneObject* sceneObject);
	void SetUniformsShaderEditor(MoravaShader* shaderEditor, Texture* texture, SceneObject* sceneObject);
	void SetUniformsShaderSkinning(MoravaShader* shaderSkinning, SceneObject* sceneObject, float runningTime);
	void SetUniformsShaderHybridAnimPBR(MoravaShader* shaderHybridAnimPBR, Texture* texture, SceneObject* sceneObject, float runningTime);
	void SetUniformsShaderWater(MoravaShader* shaderWater, SceneObject* sceneObject, glm::mat4& projectionMatrix);
	void SwitchOrthographicView(Window* mainWindow, glm::mat4& projectionMatrix);
	glm::mat4 CalculateRenderTransform(SceneObject* sceneObject);
	virtual bool IsWaterOnScene() override;
	void UpdateLightDirection(glm::quat rotation);
    void ResizeViewport(glm::vec2 viewportPanelSize);

public:
	bool m_IsViewportEnabled;
	bool m_GizmoOrthoProjection;

private:
	int m_ImGuiMainViewportX;
	int m_ImGuiMainViewportY;

	struct Viewport
	{
		int X;
		int Y;
		int Width;
		int Height;
		int MouseX;
		int MouseY;
	} m_ImGuiViewport;

	MaterialWorkflowPBR* m_MaterialWorkflowPBR;

	glm::vec2 m_ViewportSize;

	Raycast* m_Raycast;
	Grid* m_Grid;
	Quad* m_Quad;
	Pivot* m_PivotScene;
	Gizmo* m_Gizmo;

	std::vector<SceneObject*> m_SceneObjects;

	// ImGui variables
	glm::vec3* m_PositionEdit;
	glm::vec3* m_RotationEdit;
	glm::vec3* m_ScaleEdit;
	glm::vec4* m_ColorEdit;
	float* m_TilingFactorEdit;
	std::string* m_TextureNameEdit;
	std::string* m_MaterialNameEdit;
	float* m_TilingFactorMaterialEdit;
	bool m_DrawGizmos;
	int m_PBR_Map_Edit;
	int m_HDRI_Edit;
	int m_HDRI_Edit_Prev;
	int m_BlurLevel;
	int m_BlurLevelPrev;

	float m_SkyboxLOD;

	ParticleSettings* m_ParticleSettingsEdit;
	ParticleSettings* m_ParticleSettingsPrev;

	SceneObjectParticleSystem* m_CurrentSOPS;

	int m_MaxInstances = 10000;

	std::vector<std::string> m_ActiveRenderPasses;

	std::vector<PointSpotLight*> m_PointSpotLights;

	unsigned int m_SelectedIndex;
	unsigned int m_ObjectInFocusPrev;

	int m_CurrentObjectTypeID; // Current Object ID (Mesh or Model)

	bool m_MouseButton_1_Prev; // previous state of GLFW_MOUSE_BUTTON_1

	// add object cooldown
	float m_CurrentTimestamp;
	float m_StartTimestamp;

	EventCooldown m_ObjectSelect;
	EventCooldown m_ObjectAdd;
	EventCooldown m_ObjectCopy;
	EventCooldown m_ObjectDelete;
	EventCooldown m_SceneSave;
	EventCooldown m_SceneLoad;
	EventCooldown m_SceneReset;
	EventCooldown m_ProjectionChange;
	EventCooldown m_ParticlesGenerate;
    EventCooldown m_ResizeViewport;

	bool m_OrthographicViewEnabled;

	glm::vec3 defaultSpawnPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	const char* m_SceneFilename = "Scenes/scene_tmp.scene";

	bool m_DirLightEnabledPrev;
	glm::vec3 m_DirLightColorPrev;

	bool m_DisplayLightSources;

	bool m_UseCubeMaps;

	std::set<int> m_FixedVertexMeshes = { MESH_TYPE_RING, MESH_TYPE_SPHERE, MESH_TYPE_TERRAIN, MESH_TYPE_WATER };
	std::set<int> m_SkinnedMeshes = { MESH_TYPE_BOB_LAMP, MESH_TYPE_ANIM_BOY };
	std::set<int> m_AnimPBRMeshes = { MESH_TYPE_M1911 };

	std::map<std::string, std::vector<glm::mat4>> m_SkinningTransforms;

	// Model for the Glass shader
	Model* m_GlassShaderModel;

	bool m_ViewportFocused;
	bool m_ViewportHovered;

	MoravaFramebuffer* m_RenderFramebuffer;

	std::map<std::string, unsigned int> m_SamplerSlots;

	BlurEffect* m_BlurEffect;

	float m_SkyboxRotationSpeed;

	// experimental section (OpenFile dialog)
	std::string m_LoadedFile;
	Texture* m_LoadedTexture;
	std::string m_LoadedTextureFilepath;
	bool m_UseLoadedTexture;

};
