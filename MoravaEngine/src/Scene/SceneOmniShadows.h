#pragma once

#include "Scene/Scene.h"


class SceneOmniShadows : public Scene
{

public:
	SceneOmniShadows();
	virtual ~SceneOmniShadows() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::Ref<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;

private:
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMeshes() override;
};
