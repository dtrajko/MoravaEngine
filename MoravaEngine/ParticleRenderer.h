#pragma once

#include "Quad.h"
#include "Shader.h"
#include "Particle.h"
#include "Camera.h"


class ParticleRenderer
{
public:
	ParticleRenderer();
	ParticleRenderer(glm::mat4 projectionMatrix);
	void CleanUp();
	void Render(std::vector<Particle*>* particles, Camera* camera);
	void UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix);
	~ParticleRenderer();

public:
	Quad* m_Quad;
	Shader* m_ShaderParticle;
};
