#pragma once

#include "Scene.h"
#include "SceneObject.h"
#include "ParticleSettings.h"
#include "ParticleTexture.h"
#include "ParticleSystemThinMatrix.h"
#include "ParticleMaster.h"


class SceneObjectParticleSystem : public SceneObject
{
public:
	SceneObjectParticleSystem();
	SceneObjectParticleSystem(bool instancedRendering, int maxInstances);
	~SceneObjectParticleSystem();

	void Update(bool enabled, glm::vec3 cameraPosition, std::map<std::string, float>* profiler_results);
	virtual void Render() override;
	void Render(glm::mat4 viewMatrix);
	void Regenerate();
	inline ParticleMaster* GetMaster() { return m_Master; };

private:
	ParticleSettings m_Settings;
	ParticleSettings m_SettingsPrev;

	// Particle System ThinMatrix
	Texture* m_Texture;
	ParticleTexture* m_ParticleTexture;
	ParticleSystemThinMatrix* m_System;
	ParticleMaster* m_Master;

	EventCooldown m_Regenerate;

	int m_MaxInstances;
};
