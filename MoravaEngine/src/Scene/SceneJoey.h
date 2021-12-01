#pragma once

#include "Scene/Scene.h"

#include "LearnOpenGL/SphereJoey.h"
#include "Material/MaterialWorkflowPBR.h"


const unsigned int SCENE_JOEY_LIGHT_COUNT = 4;

class SceneJoey : public Scene
{
public:
	SceneJoey();
	virtual ~SceneJoey() override {};

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;

private:
	virtual void SetupMaterials() override;
	virtual void SetupModels() override;
	virtual void SetupTextures() override {};
	virtual void SetSkybox() override {};

	void SetupShaders();
	void SetupGeometry();
	void SetupLights();

public:
	glm::vec3 m_LightPositions[SCENE_JOEY_LIGHT_COUNT];
	glm::vec3 m_LightColors[SCENE_JOEY_LIGHT_COUNT];
	float m_EmissiveFactor;
	float m_MetalnessFactor;
	float m_RoughnessFactor;
	bool m_IsRotating;
	float m_RotationFactor;

private:
	H2M::RefH2M<MoravaShader> m_Shader_PBR;
	H2M::RefH2M<MoravaShader> m_Shader_PBR_MRE;
	H2M::RefH2M<MoravaShader> m_ShaderBackground;
	H2M::RefH2M<MoravaShader> m_ShaderBlurHorizontal;
	H2M::RefH2M<MoravaShader> m_ShaderBlurVertical;

	glm::vec3 m_CameraPosition;
	glm::vec3 m_LightPositionOffset[SCENE_JOEY_LIGHT_COUNT];
	glm::vec3 m_LightColorsNormal[SCENE_JOEY_LIGHT_COUNT];

	MaterialWorkflowPBR* m_MaterialWorkflowPBR;
	SphereJoey* m_SphereJoey;
	float m_Timestep = 0.0f;

	float m_MaterialSpecular = 1.0f;
	float m_MaterialShininess = 256.0f;

	int m_HDRI_Edit;
	int m_HDRI_Edit_Prev;
	int m_BlurLevel;
	int m_BlurLevelPrev;
	float m_SkyboxLOD;

};
