#pragma once

#include "ParticleTexture.h"
#include "ParticleMaster.h"

#include "glm/glm.hpp"


class ParticleMaster;

class Particle
{
public:
	Particle();
	Particle(ParticleTexture* texture, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale,
		glm::vec3 velocity, float gravity, float lifeLength, ParticleMaster* particleMaster);
	inline const glm::vec3 GetPosition() const { return m_Position; };
	inline const glm::vec3 GetRotation() const { return m_Rotation; };
	inline const glm::vec3 GetScale() const { return m_Scale; };
	inline ParticleTexture* GetTexture() const { return m_Texture; };
	inline const glm::vec2 GetTexOffset1() const { return m_TexOffset1; };
	inline const glm::vec2 GetTexOffset2() const { return m_TexOffset2; };
	inline const float GetBlend() const { return m_Blend; };
	inline const float GetDistance() const { return m_Distance; };
	bool Update(glm::vec3 cameraPosition);
	~Particle();

private:
	void UpdateTextureCoordInfo();
	void SetTextureOffset(glm::vec2* offset, int index);

private:
	ParticleTexture* m_Texture;

	glm::vec2 m_TexOffset1;
	glm::vec2 m_TexOffset2;
	float m_Blend;

	glm::vec3 m_Position;
	glm::vec3 m_Rotation;
	glm::vec3 m_Scale;

	glm::vec3 m_Velocity;
	float m_Gravity;
	float m_WorldGravity;
	float m_LifeLength;
	float m_ElapsedTime = 0;
	float m_Distance;

	glm::vec3 m_Change;

};
