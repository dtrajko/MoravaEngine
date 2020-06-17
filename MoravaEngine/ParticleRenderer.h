#pragma once

#include "Quad.h"
#include "Shader.h"
#include "Particle.h"
#include "Camera.h"


class ParticleRenderer
{
public:
	ParticleRenderer();
	void Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, Camera* camera);
	void RenderBefore();
	void RenderAfter();
	void UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix);
	void LoadTexCoordInfo(glm::vec2 texOffset1, glm::vec2 texOffset2, int numRows, float blendFactor);
	void BindTexture(ParticleTexture* particleTexture);
	void CleanUp();
	~ParticleRenderer();

private:
	Quad* m_Quad;
	Shader* m_ShaderParticle;

};
