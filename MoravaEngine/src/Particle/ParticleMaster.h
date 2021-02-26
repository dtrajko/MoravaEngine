#pragma once

#include "Particle.h"
#include "ParticleRenderer.h"
#include "../../CameraController.h"

#include <map>

class Particle;
class ParticleRenderer;

class ParticleMaster
{
public:
	ParticleMaster();
	ParticleMaster(bool instancedRendering, int maxInstances);
	virtual ~ParticleMaster();

	void Update(glm::vec3 cameraPosition);
	void Render(glm::mat4 viewMatrix);
	void addParticle(Particle* particle);
	std::map<int, int> GetCounts();

private:
	std::map<ParticleTexture*, std::vector<Particle*>*>* m_Particles;
	ParticleRenderer* m_Renderer;
};
