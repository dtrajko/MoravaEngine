#pragma once

#include "ParticleTexture.h"

#include <glm/glm.hpp>


class ParticleSystemThinMatrix
{
public:
	ParticleSystemThinMatrix();
	ParticleSystemThinMatrix(ParticleTexture* texture, float PPS, float speed, float gravityComplient, float lifeLength);
	void GeneratePatricles(glm::vec3 systemCenter);
	void EmitParticle(glm::vec3 center);
	~ParticleSystemThinMatrix();

private:
	ParticleTexture* m_Texture;

	float m_PPS;
	float m_Speed;
	float m_GravityComplient;
	float m_LifeLength;

};
