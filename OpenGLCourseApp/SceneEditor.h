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

	int m_Selected;

	// add object cooldown
	float m_AddObjectLastTime = 0.0f;
	float m_AddObjectCooldown = 1.0f;

};
