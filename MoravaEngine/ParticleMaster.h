#pragma once

#include "Particle.h"
#include "ParticleRenderer.h"
#include "Camera.h"

#include <map>


class ParticleMaster
{
public:
	ParticleMaster();
	static void Init();
	static void Update();
	static void Render(Camera* camera);
	static void CleanUp();
	static void addParticle(Particle* particle);
	~ParticleMaster();

private:
	static std::map<ParticleTexture*, std::vector<Particle*>*> m_Particles;
	static ParticleRenderer* m_Renderer;
};
