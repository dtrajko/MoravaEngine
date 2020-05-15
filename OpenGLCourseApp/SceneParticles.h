#pragma once

#include "Scene.h"

#include "ParticleSystem.h"


class SceneParticles : public Scene
{

public:
	SceneParticles();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) override;
	virtual void Render(glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	inline float GetFOV() { return m_FOV; };
	virtual ~SceneParticles() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	void SetupParticles();

	float m_FOV = 60.0f;

	// Particle System
	ParticleSystem m_ParticleSystem;
	ParticleProps m_Particle;

};
