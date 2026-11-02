#include "Particle/ParticleSystemThinMatrix.h"

#include "Core/Math.h"
#include "Core/Timer.h"
#include "Particle/Particle.h"


ParticleSystemThinMatrix::ParticleSystemThinMatrix()
{
}

ParticleSystemThinMatrix::ParticleSystemThinMatrix(ParticleTexture* texture, int PPS, glm::vec3 direction, float intensity, float gravityComplient, float lifeLength, float diameter)
{
	m_Texture = texture;

	m_PPS = PPS;
	m_Diameter = diameter;
	m_Direction = direction;
	m_Intensity = intensity;
	m_GravityComplient = gravityComplient;
	m_LifeLength = lifeLength;
}

void ParticleSystemThinMatrix::GenerateParticles(glm::vec3 position, glm::vec3 scale, ParticleMaster* particleMaster)
{
	glm::vec3 systemPosition = position;

	float delta = Timer::Get()->GetDeltaTime();
	float particlesToCreate = m_PPS * delta;
	int count = (int)std::floor(particlesToCreate);
	double integerPart;
	float partialParticle = (float)modf(particlesToCreate, &integerPart);

	for (int i = 0; i < count; i++) {
		systemPosition = CalculateRandomAreaPosition(position, scale);
		EmitParticle(systemPosition, particleMaster);
	}

	float rand = (float)std::rand() / (float)RAND_MAX;
	if (rand < partialParticle) {
		systemPosition = CalculateRandomAreaPosition(position, scale);
		EmitParticle(systemPosition, particleMaster);
	}
}

void ParticleSystemThinMatrix::EmitParticle(glm::vec3 position, ParticleMaster* particleMaster)
{
	float diameterMultiplier = 5.0f;
	float randNormX = ((float)std::rand() / (float)RAND_MAX) * 1.0f - 0.5f;
	float randNormY = ((float)std::rand() / (float)RAND_MAX) * 1.0f - 0.5f;
	float randNormZ = ((float)std::rand() / (float)RAND_MAX) * 1.0f - 0.5f;
	float randX = randNormX + m_Direction.x * m_Intensity;
	float randY = randNormY + m_Direction.y * m_Intensity;
	float randZ = randNormZ + m_Direction.z * m_Intensity;
	float dirX = randX * m_Diameter * diameterMultiplier;
	float dirY = randY * m_Diameter * diameterMultiplier;
	float dirZ = randZ * m_Diameter * diameterMultiplier;
	glm::vec3 velocity = glm::vec3(dirX, dirY, dirZ);
	new Particle(m_Texture, position, glm::vec3(0.0f), glm::vec3(1.0f), velocity, m_GravityComplient, m_LifeLength, particleMaster);
}

glm::vec3 ParticleSystemThinMatrix::CalculateRandomAreaPosition(glm::vec3 position, glm::vec3 scale)
{
	glm::vec3 randomAreaPosition = position;
	glm::vec3 randomPosition = position;

	// vector3 of random values -1 to 1
	glm::vec3 randVec = glm::vec3(
		((float)std::rand() / (float)RAND_MAX) * 1.0f - 0.5f,
		((float)std::rand() / (float)RAND_MAX) * 1.0f - 0.5f,
		((float)std::rand() / (float)RAND_MAX) * 1.0f - 0.5f);
	glm::vec3 randomScale = scale * randVec;

	randomAreaPosition = randomPosition + randomScale;

	return randomAreaPosition;
}

ParticleSystemThinMatrix::~ParticleSystemThinMatrix()
{
}
