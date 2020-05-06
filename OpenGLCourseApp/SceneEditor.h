#pragma once

#include "Scene.h"

#include "Quad.h"
#include "Raycast.h"
#include "AABB.h"
#include "Pivot.h"


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
	virtual ~SceneEditor() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	Raycast* m_Raycast;
	Quad* m_Quad;

public:
	glm::vec3 m_Position_1;
	glm::vec3 m_Position_2;

	glm::vec3 m_Scale_1;
	glm::vec3 m_Scale_2;

	glm::mat4 m_Transform_1;
	glm::mat4 m_Transform_2;

	glm::vec4 m_Color_1;
	glm::vec4 m_Color_2;

	bool m_IsSelected_1;
	bool m_IsSelected_2;

	glm::vec3* m_PositionEdit;
	glm::vec3* m_ScaleEdit;
	glm::vec4* m_ColorEdit;

	int m_Selected;

	AABB* m_AABB_1;
	AABB* m_AABB_2;

	Pivot* m_Pivot_1;
	Pivot* m_Pivot_2;

};
