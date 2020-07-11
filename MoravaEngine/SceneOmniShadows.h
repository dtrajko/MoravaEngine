#pragma once

#include "Scene.h"


class SceneOmniShadows : public Scene
{

public:
	SceneOmniShadows();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual ~SceneOmniShadows() override;

};
