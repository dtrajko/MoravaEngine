#pragma once

#include "H2M/Core/Events/KeyEventH2M.h"
#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Scene/EntityH2M.h"

#include "Framebuffer/MoravaFramebuffer.h"
#include "Mesh/Grid.h"
#include "Scene/Scene.h"
#include "Texture/MoravaTexture.h"

#include "Mono/GameInvoker/CsGame.h"

#include <map>
#include <string>


class EnvMapVulkanEditorLayer;

class SceneEnvMapVulkan : public Scene
{

public:
	SceneEnvMapVulkan();
	virtual ~SceneEnvMapVulkan() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
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

	bool OnKeyPressed(H2M::KeyPressedEventH2M& e);

	// Temporary/experimental
	virtual void OnEntitySelected(H2M::EntityH2M entity) override;

private:
	EnvMapVulkanEditorLayer* m_EnvMapVulkanEditorLayer;

	H2M::RefH2M<MoravaShader> m_ShaderBackground;
	H2M::RefH2M<MoravaShader> m_ShaderBasic;

	Grid* m_Grid;
	Pivot* m_PivotScene;

	CsGame* m_CsGame;

	friend class EnvMapVulkanEditorLayer;

};
