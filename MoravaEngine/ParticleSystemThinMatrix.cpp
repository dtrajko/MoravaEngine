#include "ParticleSystemThinMatrix.h"
#include "Particle.h"
#include "Timer.h"


ParticleSystemThinMatrix::ParticleSystemThinMatrix()
{

}

ParticleSystemThinMatrix::ParticleSystemThinMatrix(float PPS, float speed, float gravityComplient, float lifeLength)
{
	m_PPS = PPS;
	m_Speed = speed;
	m_GravityComplient = gravityComplient;
	m_LifeLength = lifeLength;
}

void ParticleSystemThinMatrix::GeneratePatricles(glm::vec3 systemCenter)
{
	float delta = Timer::Get()->GetDeltaTime();
	float particlesToCreate = m_PPS * delta;
	int count = (int)std::floor(particlesToCreate);
	float partialParticle = (float)((int)particlesToCreate % 1);
	for (int i = 0; i < count; i++) {
		EmitParticle(systemCenter);
	}
	if ((std::rand() / RAND_MAX) < partialParticle) {
		EmitParticle(systemCenter);
	}
}

void ParticleSystemThinMatrix::EmitParticle(glm::vec3 center)
{
	float diameter = 0.4f;
	float randX = (float)std::rand() / (float)RAND_MAX;
	float randZ = (float)std::rand() / (float)RAND_MAX;
	float dirX = (randX * 2.0f - 1.0f) * diameter;
	float dirZ = (randZ * 2.0f - 1.0f) * diameter;
	glm::vec3 velocityNorm = glm::vec3(dirX, 1, dirZ);

	velocityNorm = glm::normalize(velocityNorm);
	glm::vec3 velocity = velocityNorm * m_Speed;

	new Particle(center, glm::vec3(0.0f), glm::vec3(1.0f), velocity, m_GravityComplient, m_LifeLength);
}

ParticleSystemThinMatrix::~ParticleSystemThinMatrix()
{
}
