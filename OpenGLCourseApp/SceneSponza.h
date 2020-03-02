#pragma once

#include "Scene.h"


class SceneSponza : public Scene
{

public:
	SceneSponza();
	virtual void Update(float timestep, LightManager* lightManager) override;
	virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager) override;
	virtual void RenderWater(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager) override;
	~SceneSponza();

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;

};
