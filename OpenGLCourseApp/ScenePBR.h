#pragma once

#include "Scene.h"


class ScenePBR : public Scene
{

public:
	ScenePBR();
	virtual void Update(float timestep, Camera* camera, LightManager& lightManager, WaterManager* waterManager) override;
	virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager) override;
	virtual void RenderWater(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager) override;
	virtual void RenderPBR(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	~ScenePBR();

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;

private:
	glm::vec3 m_LightPosition;
	glm::vec3 m_LightColor;
	glm::vec3 m_Albedo;
	float m_AmbientOcclusion;
	float m_AmbientIntensity;

};
