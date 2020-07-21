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
	SceneObjectParticleSystem(bool instancedRendering, int maxInstances, CameraController* cameraController);
	~SceneObjectParticleSystem();

	void Update(bool enabled, std::map<std::string, float>* profiler_results);
	virtual void Render() override;
	inline ParticleMaster* GetMaster() { return m_Master; };
	inline ParticleSettings* GetSettings() { return &m_Settings; };

private:
	void Regenerate();

private:
	CameraController* m_CameraController;

	ParticleSettings m_Settings;
	ParticleSettings m_SettingsPrev;

	// Particle System ThinMatrix
	ParticleTexture* m_ParticleTexture;
	ParticleSystemThinMatrix* m_System;
	ParticleMaster* m_Master;
	EventCooldown m_Regenerate;
	int m_MaxInstances;
};
