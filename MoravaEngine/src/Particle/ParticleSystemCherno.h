#pragma once

#include "../../Quad.h"
#include "../../Shader.h"

#include "Camera/Camera.h"

#include <vector>


struct ParticleProps
{
	glm::vec2 Position;
	glm::vec2 Velocity, VelocityVariation;
	glm::vec4 ColorBegin, ColorEnd;
	float SizeBegin, SizeEnd, SizeVariation;
	float LifeTime = 1.0f;
};

class ParticleSystemCherno
{
public:
	ParticleSystemCherno();
	ParticleSystemCherno(uint32_t maxParticles);

	void OnStart();
	void OnUpdate(float ts);
	void OnRender(Camera* camera, Shader* shader);
	void DrawRotatedQuad(glm::vec3 position, glm::vec3 size, float rotation, glm::vec4 color, Shader* shader);

	void Emit(const ParticleProps& particleProps);
private:
	struct Particle
	{
		glm::vec2 Position;
		glm::vec2 Velocity;
		glm::vec4 ColorBegin, ColorEnd;
		float Rotation = 0.0f;
		float SizeBegin, SizeEnd;

		float LifeTime = 1.0f;
		float LifeRemaining = 0.0f;

		bool Active = false;
	};
	std::vector<Particle> m_ParticlePool;
	uint32_t m_PoolIndex;

	Shader* m_Shader;
	Quad* m_Quad;
};
