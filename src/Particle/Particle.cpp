#include "Particle/Particle.h"

#include "Core/Timer.h"

#include "glm/gtc/matrix_transform.hpp"


Particle::Particle()
{
	m_Texture = nullptr;

	m_TexOffset1 = glm::vec2(0.0f);
	m_TexOffset2 = glm::vec2(0.0f);
	m_Blend = 1.0f;

	m_Position = glm::vec3(0.0f);
	m_Rotation = glm::vec3(0.0f);
	m_Scale = glm::vec3(1.0f);

	m_Velocity = glm::vec3(0.0f);
	m_Gravity = 0.0f;
	m_LifeLength = 0.0f;

	m_WorldGravity = 0.0f;
}

Particle::Particle(ParticleTexture* texture, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, 
	glm::vec3 velocity, float gravity, float lifeLength, ParticleMaster* particleMaster)
	: Particle()
{
	m_Texture = texture;

	m_Position = position;
	m_Rotation = rotation;
	m_Scale = scale;

	m_Velocity = velocity;
	m_Gravity = gravity;
	m_LifeLength = lifeLength;

	particleMaster->addParticle(this);
}

bool Particle::Update(glm::vec3 cameraPosition)
{
	m_Velocity.y += (m_WorldGravity + m_Gravity) * Timer::Get()->GetDeltaTime();
	m_Change = glm::vec3(m_Velocity);
	m_Change *= Timer::Get()->GetDeltaTime();
	m_Position += m_Change;
	m_Distance = glm::distance(cameraPosition, m_Position);
	UpdateTextureCoordInfo();
	m_ElapsedTime += Timer::Get()->GetDeltaTime();
	return m_ElapsedTime < m_LifeLength;
}

void Particle::UpdateTextureCoordInfo()
{
	float lifeFactor = m_ElapsedTime / m_LifeLength;
	int stageCount = m_Texture->GetNumberOfRows() * m_Texture->GetNumberOfRows();
	float atlasProgression = lifeFactor * stageCount;
	int index1 = (int)floor(atlasProgression);
	int index2 = index1 < stageCount - 1 ? index1 + 1 : index1;
	double integerPart;
	m_Blend  = (float)modf(atlasProgression, &integerPart);
	SetTextureOffset(&m_TexOffset1, index1);
	SetTextureOffset(&m_TexOffset2, index2);
}

void Particle::SetTextureOffset(glm::vec2* offset, int index)
{
	int column = index % m_Texture->GetNumberOfRows();
	int row = index / m_Texture->GetNumberOfRows();
	offset->x = (float) column / m_Texture->GetNumberOfRows();
	offset->y = (float)row / m_Texture->GetNumberOfRows();
}

Particle::~Particle()
{
}
