#pragma once

#include "Scene.h"
#include "SceneObject.h"
#include "ParticleSettings.h"
#include "ParticleTexture.h"
#include "ParticleSystemThinMatrix.h"


class SceneObjectParticleSystem : public SceneObject
{
public:
	SceneObjectParticleSystem();
	void Update(bool enabled, glm::vec3 cameraPosition, std::map<std::string, float>* profiler_results);
	virtual void Render() override;
	void Render(glm::mat4 viewMatrix, std::map<std::string, float>* profiler_results);
	~SceneObjectParticleSystem();

private:
	void Regenerate();

private:
	ParticleSettings m_Settings;
	ParticleSettings m_SettingsPrev;

	// Particle System ThinMatrix
	Texture* m_Texture;
	ParticleTexture* m_ParticleTexture;
	ParticleSystemThinMatrix* m_System;

	EventCooldown m_Regenerate;

	static int m_MaxInstances;
};
