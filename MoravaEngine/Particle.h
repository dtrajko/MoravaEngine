#pragma once

#include "glm/glm.hpp"


class Particle
{
public:
	Particle();
	Particle(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::vec3 velocity, float gravity, float lifeLength);
	inline const glm::vec3 GetPosition() const { return m_Position; };
	inline const glm::vec3 GetRotation() const { return m_Rotation; };
	inline const glm::vec3 GetScale() const { return m_Scale; };
	bool Update();
	~Particle();

private:
	glm::vec3 m_Position;
	glm::vec3 m_Rotation;
	glm::vec3 m_Scale;

	glm::vec3 m_Velocity;
	float m_Gravity;
	float m_WorldGravity;
	float m_LifeLength;
	float m_ElapsedTime = 0;

};
