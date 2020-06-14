#include "Particle.h"

#include "glm/gtc/matrix_transform.hpp"

#include "Timer.h"


Particle::Particle()
{
	m_Position = glm::vec3(0.0f);
	m_Rotation = glm::vec3(0.0f);
	m_Scale = glm::vec3(1.0f);

	m_Velocity = glm::vec3(0.0f);
	m_Gravity = 0.0f;
	m_LifeLength = 0.0f;

	m_WorldGravity = -50.0f;
}

Particle::Particle(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::vec3 velocity, float gravity, float lifeLength)
	: Particle()
{
	m_Position = position;
	m_Rotation = rotation;
	m_Scale = scale;

	m_Velocity = velocity;
	m_Gravity = gravity;
	m_LifeLength = lifeLength;
}

bool Particle::Update()
{
	m_Velocity.y += m_WorldGravity * m_Gravity * Timer::Get()->GetDeltaTime();
	glm::vec3 change = glm::vec3(m_Velocity);
	change *= Timer::Get()->GetDeltaTime();
	m_Position += change;
	m_ElapsedTime += Timer::Get()->GetDeltaTime();
	return m_ElapsedTime < m_LifeLength;
}

Particle::~Particle()
{
}
