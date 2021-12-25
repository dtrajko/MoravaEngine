#pragma once

#include "Scene/Scene.h"


class SceneEiffel : public Scene
{

public:
	SceneEiffel();
	virtual ~SceneEiffel() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;

	// virtual void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow) override;

	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
	virtual void RenderWater(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;

private:
	virtual void SetSkybox() override;
	virtual void SetupTextures() override;
	virtual void SetupModels() override;
	virtual void SetupMeshes() override;

	glm::vec3 m_LightDirection;
	glm::vec3 m_LightColor;

	// Parameters for light projection orthogonal matrix
	float m_OrthoLeft   = -32.0f;
	float m_OrthoRight  =  32.0f;
	float m_OrthoBottom = -32.0f;
	float m_OrthoTop    =  32.0f;
	float m_OrthoNear   = -32.0f;
	float m_OrthoFar    =  32.0f;

	bool m_ShowWindowSettings = true;
	bool m_ShowWindowFramebuffers = true;

};
