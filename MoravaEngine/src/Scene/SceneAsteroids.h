#pragma once

#include "LearnOpenGL/ModelJoey.h"
#include "Scene/Scene.h"


class SceneAsteroids : public Scene
{

public:
	SceneAsteroids();
	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Hazel::Ref<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
	inline std::map<std::string, ModelJoey*> GetModels() const { return models; };
	inline float GetFOV() { return m_FOV; };
	virtual ~SceneAsteroids() override;

private:
	virtual void SetSkybox() override;
	virtual void SetupModels() override;

	void SetupShaders();
	void SetupGeometry();

private:
	Hazel::Ref<MoravaShader> m_ShaderAsteroids;
	Hazel::Ref<MoravaShader> m_ShaderPlanet;

	unsigned int quadVAO;
	unsigned int quadVBO;

	// Asteroid field
	unsigned int amount = 50000;
	glm::mat4* modelMatrices;

	std::map<std::string, ModelJoey*> models;

	float m_FOV = 60.0f;

};
