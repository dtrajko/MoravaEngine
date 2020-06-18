#include "ParticleMaster.h"

#include "InsertionSort.h"
#include "ParticleRendererInstanced.h"


std::map<ParticleTexture*, std::vector<Particle*>*> ParticleMaster::m_Particles;
ParticleRenderer* ParticleMaster::m_Renderer;

ParticleMaster::ParticleMaster()
{
}

void ParticleMaster::Init()
{
	m_Particles = std::map<ParticleTexture*, std::vector<Particle*>*>();
	m_Renderer = new ParticleRendererInstanced();
}

void ParticleMaster::Update(glm::vec3 cameraPosition)
{
	bool stillAlive = true;
	for (auto it_map = m_Particles.begin(); it_map != m_Particles.end(); it_map++)
	{
		// a secondary vector to copy only alive particles to
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

	// deallocate empty map elements
	for (auto it_map = m_Particles.cbegin(); it_map != m_Particles.cend();)
	{
		if (it_map->second->size() == 0) {
			m_Particles.erase(it_map++);
		}
		else {
			++it_map;
		}
	}
}

void ParticleMaster::Render(glm::mat4 viewMatrix)
{
	m_Renderer->Render(&m_Particles, viewMatrix);
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
