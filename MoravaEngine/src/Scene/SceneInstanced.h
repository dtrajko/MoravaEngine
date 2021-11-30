#pragma once

#include "Scene/Scene.h"


class SceneInstanced : public Scene
{

public:
	SceneInstanced();
	virtual ~SceneInstanced() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::Ref<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;

	void SetupGeometry();
	void SetupShaders();

private:
	H2M::Ref<MoravaShader> m_ShaderInstanced;

	unsigned int quadVAO;
	unsigned int quadVBO;

};
