#pragma once

#include "Scene.h"


class SceneEiffel : public Scene
{

public:
	SceneEiffel();
	virtual void Update(float timestep, LightManager& lightManager, WaterManager* waterManager) override;
	virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager) override;
	virtual void RenderWater(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager) override;
	~SceneEiffel();

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;
	virtual void SetupMeshes() override;

	glm::vec3 m_LightDirection;

};
