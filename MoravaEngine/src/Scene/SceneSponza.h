#pragma once

#include "Scene/Scene.h"


class SceneSponza : public Scene
{

public:
	SceneSponza();
	virtual ~SceneSponza() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;

	virtual void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow) override;
	virtual void RenderImGuiMenu(Window* mainWindow, ImGuiDockNodeFlags dockspaceFlags) override;

	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;

private:
	virtual void SetSkybox() override;
	virtual void SetupTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	bool m_ShowWindowLights = true;
	bool m_ShowWindowSceneSettings = true;

};
