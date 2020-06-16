#include "ParticleMaster.h"
#include "InsertionSort.h"


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

void ParticleMaster::Update(glm::vec3 cameraPosition)
{
	bool stillAlive = true;
	for (auto it_map = m_Particles.begin(); it_map != m_Particles.end(); it_map++)
	{
		std::vector<Particle*>* secondVec = new std::vector<Particle*>();

		for (int i = 0; i < it_map->second->size(); i++)
		{
			stillAlive = it_map->second->at(i)->Update(cameraPosition);
			if (stillAlive) {
				secondVec->push_back(it_map->second->at(i));
			}
		}

		// InsertionSort::SortHighToLow(secondVec);
		it_map->second->clear();
		it_map->second = secondVec;
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

	// printf("ParticleMaster::addParticle particleVec = %zu\n", particleVec->size());
}

void ParticleMaster::CleanUp()
{
	m_Renderer->CleanUp();
}

ParticleMaster::~ParticleMaster()
{
}
