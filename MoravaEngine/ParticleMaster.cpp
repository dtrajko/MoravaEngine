#include "ParticleMaster.h"


std::vector<Particle*> ParticleMaster::m_Particles;
ParticleRenderer* ParticleMaster::m_Renderer;

ParticleMaster::ParticleMaster()
{
}

void ParticleMaster::Init(glm::mat4 projectionMatrix)
{
	m_Particles = std::vector<Particle*>();
	m_Renderer = new ParticleRenderer(projectionMatrix);
}

void ParticleMaster::Update()
{
	std::vector<Particle*>::iterator it = m_Particles.begin();
	bool stillAlive = true;

	for (it = m_Particles.begin(); it != m_Particles.end(); ++it)
	{
		stillAlive = (*it)->Update();
		if (!stillAlive) {
			// m_Particles.erase(it);
		}
	}
}

void ParticleMaster::Render(Camera* camera)
{
	m_Renderer->Render(&m_Particles, camera);
}

void ParticleMaster::addParticle(Particle* particle)
{
	m_Particles.push_back(particle);
}

void ParticleMaster::CleanUp()
{
	m_Renderer->CleanUp();
}

ParticleMaster::~ParticleMaster()
{
}
