#pragma once

#include "ParticleTexture.h"
#include "AABB.h"

#include <glm/glm.hpp>


class ParticleSystemThinMatrix
{
public:
	ParticleSystemThinMatrix();
	ParticleSystemThinMatrix(ParticleTexture* texture, int PPS, glm::vec3 direction, float intensity, float gravityComplient, float lifeLength, float diameter);
	void GeneratePatricles(glm::vec3 position, glm::vec3 scale);
	void EmitParticle(glm::vec3 position);
	glm::vec3 CalculateRandomAreaPosition(glm::vec3 position, glm::vec3 scale);
	~ParticleSystemThinMatrix();

private:
	ParticleTexture* m_Texture;
	AABB* m_AABB;

	glm::vec3 m_Position;
	glm::vec3 m_Rotation;
	glm::vec3 m_Scale;

	glm::vec3 m_Direction;
	float m_Intensity;
	float m_Diameter;
	int m_PPS;
	float m_GravityComplient;
	float m_LifeLength;

};
