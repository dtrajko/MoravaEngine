#pragma once

#include "Scene.h"


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

	// ImGuizmo
	float GetSnapValue();

	glm::mat4 m_CubeTransform;
	int m_GizmoType;

};
