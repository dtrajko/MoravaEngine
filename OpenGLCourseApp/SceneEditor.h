#pragma once

#include "Scene.h"

#include "Quad.h"
#include "Raycast.h"
#include "AABB.h"
#include "Pivot.h"
#include "Grid.h"
#include "Gizmo.h"
#include "TextureCubeMap.h"
#include "MaterialWorkflowPBR.h"


const int SKYBOX_DAY   = 0;
const int SKYBOX_NIGHT = 1;

const int ACTION_ADD_MESH  = 0;
const int ACTION_ADD_MODEL = 1;

const int MODEL_STONE_CARVED = 0;
const int MODEL_OLD_STOVE    = 1;

struct PointSpotLight
{
	bool enabled;
	glm::vec3 position;
	glm::vec3 color;
	float exponent;
	float linear;
	float constant;
};


class SceneEditor : public Scene
{
public:
	SceneEditor();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	void SetGeometry();
	void CleanupGeometry();
	inline Raycast* GetRaycast() const { return m_Raycast; };
	SceneObject* CreateNewSceneObject();
	void AddSceneObject();
	void CopySceneObject(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	void DeleteSceneObject(Window& mainWindow, std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	void SelectNextFromMultipleObjects(std::vector<SceneObject*>* sceneObjects, unsigned int& selectedIndex);
	bool IsObjectSelected(unsigned int objectIndex);
	void SaveScene();
	void LoadScene();
	void ResetScene();
	Mesh* CreateNewPrimitive(int meshTypeID, glm::vec3 scale);
	Model* AddNewModel(int modelID, glm::vec3 scale);
	virtual ~SceneEditor() override;

private:
	virtual void SetTextures()    override;
	virtual void SetupMaterials() override;
	virtual void SetupMeshes()    override;
	virtual void SetupModels()    override;
	virtual void SetSkybox()      override;

private:
	MaterialWorkflowPBR* m_MaterialWorkflowPBR;

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
	bool* m_UseTextureEdit;
	float* m_TilingFactorEdit;
	std::string* m_TextureNameEdit;
	std::string* m_MaterialNameEdit;
	float* m_TilingFactorMaterialEdit;
	bool m_DrawScenePivot;

	float m_MaterialSpecular = 1.0f;
	float m_MaterialShininess = 256.0f;

	std::vector<PointSpotLight*> m_PointSpotLights;

	unsigned int m_SelectedIndex;
	unsigned int m_ObjectInFocusPrev;

	int m_ActionAddType;
	int m_CurrentMeshTypeID; // Current Mesh ID
	int m_CurrentModelID;    // Current Model ID

	bool m_MouseButton_1_Prev; // previous state of GLFW_MOUSE_BUTTON_1

	// add object cooldown
	float m_CurrentTimestamp;

	EventCooldown m_ObjectSelect;
	EventCooldown m_ObjectAdd;
	EventCooldown m_ObjectCopy;
	EventCooldown m_ObjectDelete;
	EventCooldown m_SceneSave;
	EventCooldown m_SceneLoad;
	EventCooldown m_SceneReset;
	EventCooldown m_ProjectionChange;

	bool m_OrthographicViewEnabled;

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
	TextureCubeMap* m_TextureCubeMapDay;
	TextureCubeMap* m_TextureCubeMapNight;

	bool m_UseCubeMaps;

};
