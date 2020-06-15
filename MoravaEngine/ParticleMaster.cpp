#include "ParticleMaster.h"


std::map<ParticleTexture*, std::vector<Particle*>*> ParticleMaster::m_Particles;
ParticleRenderer* ParticleMaster::m_Renderer;

ParticleMaster::ParticleMaster()
{
}

void ParticleMaster::Init()
{
	m_Particles = std::map<ParticleTexture*, std::vector<Particle*>*>();
	m_Renderer = new ParticleRenderer();
}

void ParticleMaster::Update()
{
	bool stillAlive = true;
	for (auto it_map = m_Particles.cbegin(); it_map != m_Particles.cend(); ++it_map)
	{
		for (auto it_vec = it_map->second->cbegin(); it_vec != it_map->second->cend();)
		{
			stillAlive = (*it_vec)->Update();
			if (!stillAlive) {
				it_map->second->erase(it_vec++);
			}
			else {
				++it_vec;
			};
		}

		// if (it_map->second->empty()) {
		// 	m_Particles.erase(it_map++);
		// } else {
		// 	++it_map;
		// }
	}
}

void ParticleMaster::Render(Camera* camera)
{
	m_Renderer->Render(&m_Particles, camera);
}

void ParticleMaster::addParticle(Particle* particle)
{
	std::vector<Particle*>* particleVec;
	std::map<ParticleTexture*, std::vector<Particle*>*>::iterator particleIt;

	particleIt = m_Particles.find(particle->GetTexture());

	if (particleIt != m_Particles.end()) {
		particleVec = particleIt->second;
	}
	else {
		particleVec = new std::vector<Particle*>();
		m_Particles.insert(std::make_pair(particle->GetTexture(), particleVec));
	}

	particleVec->push_back(particle);

	printf("ParticleMaster::addParticle particleVec = %zu\n", particleVec->size());
}

void ParticleMaster::CleanUp()
{
	m_Renderer->CleanUp();
}

ParticleMaster::~ParticleMaster()
{
}
