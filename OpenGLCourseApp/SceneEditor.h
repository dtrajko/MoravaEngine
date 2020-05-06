#pragma once

#include "Scene.h"

#include "Quad.h"
#include "Raycast.h"
#include "AABB.h"
#include "Pivot.h"


struct SceneObject
{
	glm::mat4 transform;
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	glm::vec4 color;
	bool isSelected;
	AABB* AABB;
	Pivot* pivot;
};


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
	void CopySceneObject(SceneObject sceneObject);
	void SelectNextFromMultipleObjects(std::vector<SceneObject> sceneObjects, unsigned int* selected);
	bool IsObjectSelected(unsigned int objectIndex);
	virtual ~SceneEditor() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	Raycast* m_Raycast;
	Quad* m_Quad;

	std::vector<SceneObject> sceneObjects;

	glm::vec3* m_PositionEdit;
	glm::vec3* m_RotationEdit;
	glm::vec3* m_ScaleEdit;
	glm::vec4* m_ColorEdit;

	unsigned int m_SelectedIndex;
	unsigned int m_ObjectInFocusPrev;

	// add object cooldown
	float m_LastTimeAdd = 0.0f;
	float m_CooldownAdd = 1.0f;

	float m_LastTimeCopy = 0.0f;
	float m_CooldownCopy = 1.0f;

	float m_LastTimeSelect = 0.0f;
	float m_CooldownSelect = 0.2f;
};
