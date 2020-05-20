#pragma once

#include "Scene.h"

#include "Quad.h"
#include "Raycast.h"
#include "AABB.h"
#include "Pivot.h"
#include "Grid.h"
#include "Gizmo.h"
#include "TextureCubeMap.h"



const int SKYBOX_DAY   = 0;
const int SKYBOX_NIGHT = 1;


class SceneEditor : public Scene
{

public:
	SceneEditor();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	void SetGeometry();
	void CleanupGeometry();
	inline Raycast* GetRaycast() const { return m_Raycast; };
	void AddSceneObject();
	void CopySceneObject(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	void DeleteSceneObject(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	void SelectNextFromMultipleObjects(std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	bool IsObjectSelected(unsigned int objectIndex);
	void SaveScene();
	void LoadScene();
	void ResetScene();
	Mesh* CreateNewPrimitive(int meshTypeID, glm::vec3 scale);
	virtual ~SceneEditor() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	Raycast* m_Raycast;
	Grid* m_Grid;
	Quad* m_Quad;
	Pivot* m_PivotScene;
	Gizmo* m_Gizmo;

	std::vector<SceneObject*> m_SceneObjects;

	glm::vec3* m_PositionEdit;
	glm::vec3* m_RotationEdit;
	glm::vec3* m_ScaleEdit;
	glm::vec4* m_ColorEdit;
	bool* m_UseTextureEdit;
	float* m_TilingFactorEdit;
	std::string* m_TextureNameEdit;

	unsigned int m_SelectedIndex;
	unsigned int m_ObjectInFocusPrev;

	int m_CurrentMeshTypeInt;

	bool m_MouseButton_1_Prev; // previous state of GLFW_MOUSE_BUTTON_1

	// add object cooldown
	float m_CurrentTimestamp = 0.0f;
	EventCooldown m_ObjectSelect = { 0.0f, 0.2f };
	EventCooldown m_ObjectAdd = { 0.0f, 1.0f };
	EventCooldown m_ObjectCopy = { 0.0f, 1.0f };
	EventCooldown m_ObjectDelete = { 0.0f, 1.0f };
	EventCooldown m_SceneSave = { 0.0f, 1.0f };
	EventCooldown m_SceneLoad = { 0.0f, 1.0f };
	EventCooldown m_SceneReset = { 0.0f, 1.0f };

	glm::vec3 defaultSpawnPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	const char* m_SceneFilename = "Scenes/scene_tmp.scene";

	bool m_DirLightEnabledPrev;
	glm::vec3 m_DirLightColorPrev;

	bool m_DisplayLightSources;

	std::vector<std::string> m_SkyboxFacesDay;
	std::vector<std::string> m_SkyboxFacesNight;

	Skybox* m_SkyboxDay;
	Skybox* m_SkyboxNight;

	int m_CurrentSkyboxInt;

	TextureCubeMap* m_TextureCubeMap;
	unsigned int m_TextureCubeMapID;

};
