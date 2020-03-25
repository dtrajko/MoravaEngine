#pragma once

#include "Scene.h"


class SceneJoey : public Scene
{

public:
	SceneJoey();
	virtual void Update(float timestep) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual ~SceneJoey() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;
	void SetupLights();

public:
	glm::vec3 m_LightPositions[4];
	glm::vec3 m_LightColors[4];
	float m_EmissiveFactor;
	float m_MetalnessFactor;
	float m_RoughnessFactor;
	bool m_IsRotating;
	float m_RotationFactor;

private:
	glm::vec3 m_CameraPosition;
	glm::vec3 m_LightPositionOffset[4];
	glm::vec3 m_LightColorsNormal[4];


};
