#pragma once

#include <glm/glm.hpp>


class ParticleSystemThinMatrix
{
public:
	ParticleSystemThinMatrix();
	ParticleSystemThinMatrix(float PPS, float speed, float gravityComplient, float lifeLength);
	void GeneratePatricles(glm::vec3 systemCenter);
	void EmitParticle(glm::vec3 center);
	~ParticleSystemThinMatrix();

private:
	float m_PPS;
	float m_Speed;
	float m_GravityComplient;
	float m_LifeLength;

};
