#include "ParticleMaster.h"


std::vector<Particle*> ParticleMaster::m_Particles;
ParticleRenderer* ParticleMaster::m_Renderer;

ParticleMaster::ParticleMaster()
{
}

void ParticleMaster::Init()
{
	m_Particles = std::vector<Particle*>();
	m_Renderer = new ParticleRenderer();
}

void ParticleMaster::Update()
{
	bool stillAlive = true;

	int index = 0;
	for (auto& particle : m_Particles)
	{
		stillAlive = particle->Update();
		if (!stillAlive && index < m_Particles.size()) {
			m_Particles.erase(m_Particles.begin() + index);
		}
		index++;
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
