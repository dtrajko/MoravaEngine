#pragma once

#include "Scene/Scene.h"


class SceneDeferredOGL : public Scene
{

public:
	SceneDeferredOGL();
	virtual ~SceneDeferredOGL() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;

	void UpdateImGui(float timestep, Window* mainWindow);

};
