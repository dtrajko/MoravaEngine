#pragma once

#include "Scene.h"


class SceneInstanced : public Scene
{

public:
	SceneInstanced();
	virtual ~SceneInstanced() override;

	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;

	void SetupGeometry();
	void SetupShaders();

private:
	Shader* m_ShaderInstanced;

	unsigned int quadVAO;
	unsigned int quadVBO;

};
