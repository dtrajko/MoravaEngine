#pragma once

#include "Scene.h"


class SceneTerrain : public Scene
{

public:
	SceneTerrain();

	virtual void Update(float timestep) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;

	~SceneTerrain();

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;

	glm::vec3 m_TerrainScale = glm::vec3(1.5f, 0.25f, 1.5f);

};
