#include "ParticleMaster.h"

std::vector<Particle*> ParticleMaster::m_Particles;
ParticleRenderer* ParticleMaster::m_Renderer;

ParticleMaster::ParticleMaster()
{
	m_Particles = std::vector<Particle*>();
}

void ParticleMaster::Init(glm::mat4 projectionMatrix)
{
	m_Renderer = new ParticleRenderer(projectionMatrix);
}

void ParticleMaster::Update()
{
	std::vector<Particle*>::iterator it;
	for (it = m_Particles.begin(); it != m_Particles.end(); ++it)
	{
		bool stillAlive = (*it)->Update();
		if (!stillAlive)
			m_Particles.erase(it);
	}
}

void ParticleMaster::Render(Camera* camera)
{
	m_Renderer->Render(&m_Particles, camera);
}

void ParticleMaster::CleanUp()
{
	m_Renderer->CleanUp();
}

void ParticleMaster::addParticle(Particle* particle)
{
	m_Particles.push_back(particle);
}

ParticleMaster::~ParticleMaster()
{
}
