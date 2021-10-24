#pragma once

#include "Hazel/Core/Events/KeyEvent.h"
#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Scene/Entity.h"

#include "Framebuffer/MoravaFramebuffer.h"
#include "Mesh/Grid.h"
#include "Scene/Scene.h"
#include "Texture/MoravaTexture.h"
#include "Mono/GameInvoker/CsGame.h"

#include <map>
#include <string>


class EnvMapEditorLayer;


class SceneHazelEnvMap : public Scene
{

public:
	SceneHazelEnvMap();
	virtual ~SceneHazelEnvMap() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Hazel::Ref<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
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

	Hazel::Ref<MoravaShader> m_ShaderBackground;
	Hazel::Ref<MoravaShader> m_ShaderBasic;

	Grid* m_Grid;
	Pivot* m_PivotScene;

	CsGame* m_CsGame;

	friend class EnvMapEditorLayer;

};
