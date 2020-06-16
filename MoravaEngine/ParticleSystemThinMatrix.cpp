#include "ParticleSystemThinMatrix.h"
#include "Particle.h"
#include "Timer.h"
#include "Math.h"


ParticleSystemThinMatrix::ParticleSystemThinMatrix()
{

}

ParticleSystemThinMatrix::ParticleSystemThinMatrix(ParticleTexture* texture, float PPS, glm::vec3 direction, float intensity, float gravityComplient, float lifeLength, float diameter)
{
	m_Texture = texture;

	m_PPS = PPS;
	m_Diameter = diameter;
	m_Direction = direction;
	m_Intensity = intensity;
	m_GravityComplient = gravityComplient;
	m_LifeLength = lifeLength;
}

void ParticleSystemThinMatrix::GeneratePatricles(glm::vec3 position, glm::vec3 scale)
{
	glm::vec3 systemPosition = position;

	float delta = Timer::Get()->GetDeltaTime();
	float particlesToCreate = m_PPS * delta;
	int count = (int)std::floor(particlesToCreate);
	float integerPart;
	float partialParticle = modf(particlesToCreate, &integerPart);

	for (int i = 0; i < count; i++) {
		systemPosition = CalculateRandomAreaPosition(position, scale);
		EmitParticle(systemPosition);
	}

	float rand = (float)std::rand() / (float)RAND_MAX;
	if (rand < partialParticle) {
		systemPosition = CalculateRandomAreaPosition(position, scale);
		EmitParticle(systemPosition);
	}
}

void ParticleSystemThinMatrix::EmitParticle(glm::vec3 position)
{
	float randX = (float)std::rand() / (float)RAND_MAX;
	float randY = (float)std::rand() / (float)RAND_MAX;
	float randZ = (float)std::rand() / (float)RAND_MAX;
	float dirX = randX * m_Diameter * m_Intensity; // * m_Direction.x;
	float dirY = randY * m_Diameter * m_Intensity; // * m_Direction.y;
	float dirZ = randZ * m_Diameter * m_Intensity; // * m_Direction.z;
	glm::vec3 velocity = glm::vec3(dirX, dirY, dirZ);
	velocity *= m_Direction;
	// printf("PSTM::Emit velocity [ %.2ff %.2ff %.2ff ]\n", velocity.x, velocity.y, velocity.z);
	new Particle(m_Texture, position, glm::vec3(0.0f), glm::vec3(1.0f), velocity, m_GravityComplient, m_LifeLength);
}

glm::vec3 ParticleSystemThinMatrix::CalculateRandomAreaPosition(glm::vec3 position, glm::vec3 scale)
{
	glm::vec3 randomAreaPosition = position;
	glm::vec3 randomPosition = position;

	// vector3 of random values -1 to 1
	glm::vec3 randVec = glm::vec3(
		((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f,
		((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f,
		((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f);
	glm::vec3 randomScale = scale * randVec;

	randomAreaPosition = randomPosition + randomScale;

	// printf("PSTM: randomScale [ %.2ff %.2ff %.2ff ] randomAreaPosition [ %.2ff %.2ff %.2ff ]\n",
	// 	randomScale.x, randomScale.y, randomScale.z, randomAreaPosition.x, randomAreaPosition.y, randomAreaPosition.z);

	return randomAreaPosition;
}

ParticleSystemThinMatrix::~ParticleSystemThinMatrix()
{
}
