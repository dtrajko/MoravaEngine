#pragma once

#include "Scene/Scene.h"

#include "Hazel/Editor/SceneHierarchyPanel.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Scene/Entity.h"

#include "EnvMap/EnvMapEditorLayer.h"
#include "Framebuffer/Framebuffer.h"
#include "Mesh/Grid.h"
#include "Texture/Texture.h"

#include <map>
#include <string>


const int MAX_LIGHTS_ENV_MAP = 4 + 4; // (4 x point lights) + (4 x spot lights)


class SceneHazelVulkan : public Scene
{

public:
	SceneHazelVulkan();
	virtual ~SceneHazelVulkan() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;
	void SetupUniforms();

private:
	virtual void SetLightManager() override;
	virtual void SetWaterManager(int width, int height) override;
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMaterials() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;

	void SetupShaders(); // Usually in Renderer* classes

	bool OnKeyPressed(KeyPressedEvent& e);

	// Temporary/experimental
	virtual void OnEntitySelected(Hazel::Entity entity) override;

private:
	std::unique_ptr<EnvMapEditorLayer> m_EnvMapEditorLayer;

	Hazel::Ref<Shader> m_ShaderBackground;
	Hazel::Ref<Shader> m_ShaderBasic;

	Grid* m_Grid;
	Pivot* m_PivotScene;

	friend class EnvMapEditorLayer;

};