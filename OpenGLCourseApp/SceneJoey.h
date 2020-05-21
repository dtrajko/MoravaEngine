#pragma once

#include "Scene.h"
#include "LearnOpenGL/SphereJoey.h"
#include "MaterialWorkflowPBR.h"


const unsigned int SCENE_JOEY_LIGHT_COUNT = 4;

class SceneJoey : public Scene
{
public:
	SceneJoey();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	inline std::map<std::string, Material*> GetMaterials() const { return m_Materials; };
	virtual ~SceneJoey() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupModels() override;
	void SetupGeometry();
	void SetupLights();

public:
	std::map<std::string, Material*> m_Materials;
	glm::vec3 m_LightPositions[SCENE_JOEY_LIGHT_COUNT];
	glm::vec3 m_LightColors[SCENE_JOEY_LIGHT_COUNT];
	float m_EmissiveFactor;
	float m_MetalnessFactor;
	float m_RoughnessFactor;
	bool m_IsRotating;
	float m_RotationFactor;

private:
	glm::vec3 m_CameraPosition;
	glm::vec3 m_LightPositionOffset[SCENE_JOEY_LIGHT_COUNT];
	glm::vec3 m_LightColorsNormal[SCENE_JOEY_LIGHT_COUNT];

	MaterialWorkflowPBR* m_MaterialWorkflowPBR;
	SphereJoey* m_SphereJoey;
	float m_Timestep = 0.0f;
};
