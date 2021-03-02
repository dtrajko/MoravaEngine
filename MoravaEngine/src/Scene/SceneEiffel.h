#pragma once

#include "Scene/Scene.h"


class SceneEiffel : public Scene
{

public:
	SceneEiffel();
	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;
	virtual ~SceneEiffel() override;

private:
	virtual void SetSkybox() override;
	virtual void SetupTextures() override;
	virtual void SetupModels() override;
	virtual void SetupMeshes() override;

	glm::vec3 m_LightDirection;
	glm::vec3 m_LightColor;

};
