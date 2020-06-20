#include "ParticleMaster.h"

#include "InsertionSort.h"
#include "ParticleRendererInstanced.h"


std::map<ParticleTexture*, std::vector<Particle*>*> ParticleMaster::m_Particles;
ParticleRenderer* ParticleMaster::m_Renderer;

ParticleMaster::ParticleMaster()
{
}

void ParticleMaster::Init(bool instancedRendering, int maxInstances)
{
	m_Particles = std::map<ParticleTexture*, std::vector<Particle*>*>();
	if (instancedRendering)
		m_Renderer = new ParticleRendererInstanced(maxInstances);
	else
		m_Renderer = new ParticleRenderer();
}

void ParticleMaster::Update(glm::vec3 cameraPosition)
{
	bool stillAlive = true;
	std::map<ParticleTexture*, std::vector<Particle*>*>::const_iterator it_map;
	for (it_map = m_Particles.cbegin(); it_map != m_Particles.cend();)
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
			it_map = m_Particles.erase(it_map++);
		}
		else {
			++it_map;
		}
	}
}

void ParticleMaster::Render(glm::mat4 viewMatrix)
{
	m_Renderer->Render(&m_Particles, viewMatrix);
	// printf("ParticleMaster::Render particlesMap.size: %zu particlesVector.size: %zu\n", m_Particles.size(), m_Particles.begin()->second->size());
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

std::map<int, int> ParticleMaster::GetCounts()
{
	std::map<int, int> counts = std::map<int, int>();
	for (auto it = m_Particles.begin(); it != m_Particles.end(); it++) {
		counts.insert(std::make_pair(it->first->GetTextureID(), it->second->size()));
	}
	return counts;
}

void ParticleMaster::CleanUp()
{
	delete m_Renderer;
}

ParticleMaster::~ParticleMaster()
{
}
