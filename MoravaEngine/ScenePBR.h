#pragma once

#include "Scene.h"


class ScenePBR : public Scene
{

public:
	ScenePBR();

	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual ~ScenePBR() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;

private:
	glm::vec3 m_CameraPosition;
	glm::vec3 m_Albedo;
	float m_Metallic;
	float m_Roughness;
	float m_AmbientOcclusion;
	float m_AmbientIntensity;

	glm::vec3 m_LightColor_0;
	glm::vec3 m_LightColor_1;
	glm::vec3 m_LightColor_2;
	glm::vec3 m_LightColor_3;
	glm::vec3 m_LightPosOffset_0;
	glm::vec3 m_LightPosOffset_1;
	glm::vec3 m_LightPosOffset_2;
	glm::vec3 m_LightPosOffset_3;

	glm::vec3 m_LightPositions[4];
	glm::vec3 m_LightColors[4];

	bool m_CerberusEnabled = true;
};
