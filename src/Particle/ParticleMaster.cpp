#include "Particle/ParticleMaster.h"

#include "Core/InsertionSort.h"
#include "Particle/ParticleRendererInstanced.h"


ParticleMaster::ParticleMaster()
{
	m_Renderer = nullptr;
	m_Particles = new std::map<ParticleTexture*, std::vector<Particle*>*>();
}

ParticleMaster::ParticleMaster(bool instancedRendering, int maxInstances)
{
	m_Particles = new std::map<ParticleTexture*, std::vector<Particle*>*>();
	if (instancedRendering)
		m_Renderer = new ParticleRendererInstanced(maxInstances);
	else
		m_Renderer = new ParticleRenderer();
}

void ParticleMaster::Update(glm::vec3 cameraPosition)
{
	bool stillAlive = true;
	std::map<ParticleTexture*, std::vector<Particle*>*>::const_iterator it_map;
	for (it_map = m_Particles->cbegin(); it_map != m_Particles->cend();)
	{
		std::vector<Particle*>* particleVector = it_map->second;
		std::vector<Particle*>::const_iterator it_vec;
		for (it_vec = particleVector->begin(); it_vec != particleVector->end(); ) {
			stillAlive = (*it_vec)->Update(cameraPosition);
			if (!stillAlive) {
				delete *it_vec;
				it_vec = particleVector->erase(it_vec);
			}
			else {
				++it_vec;
			}
		}

		if (particleVector->size() == 0) {
			it_map = m_Particles->erase(it_map++);
		}
		else {
			++it_map;
		}
	}
}

void ParticleMaster::Render(glm::mat4 viewMatrix)
{
	m_Renderer->Render(m_Particles, viewMatrix);
}

void ParticleMaster::addParticle(Particle* particle)
{
	std::vector<Particle*>* particleVec;
	std::map<ParticleTexture*, std::vector<Particle*>*>::iterator particleIt;

	particleIt = m_Particles->find(particle->GetTexture());

	if (particleIt != m_Particles->end()) {
		particleVec = particleIt->second;
	}
	else {
		particleVec = new std::vector<Particle*>();
		m_Particles->insert(std::make_pair(particle->GetTexture(), particleVec));
	}

	particleVec->push_back(particle);
}

std::map<int, int> ParticleMaster::GetCounts()
{
	std::map<int, int> counts = std::map<int, int>();
	for (auto it = m_Particles->begin(); it != m_Particles->end(); it++) {
		counts.insert(std::make_pair(it->first->GetTextureID(), (int)it->second->size()));
	}
	return counts;
}

ParticleMaster::~ParticleMaster()
{
	delete m_Particles;
	delete m_Renderer;
}
