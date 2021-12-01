#pragma once

#include "Editor/SceneObject.h"
#include "Particle/ParticleSettings.h"
#include "Particle/ParticleTexture.h"
#include "Particle/ParticleSystemThinMatrix.h"
#include "Particle/ParticleMaster.h"
#include "Scene/Scene.h"


class SceneObjectParticleSystem : public SceneObject
{
public:
	SceneObjectParticleSystem();
	SceneObjectParticleSystem(bool instancedRendering, int maxInstances, H2M::CameraH2M* camera, CameraController* cameraController);
	~SceneObjectParticleSystem();

	void Update(bool enabled, std::map<std::string, float>* profiler_results);
	virtual void Render() override;
	inline ParticleMaster* GetMaster() { return m_Master; };
	inline ParticleSettings* GetSettings() { return &m_Settings; };

private:
	void Regenerate();

private:
	H2M::CameraH2M* m_Camera;
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
