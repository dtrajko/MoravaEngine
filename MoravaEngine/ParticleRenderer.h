#pragma once

#include "Quad.h"
#include "QuadInstanced.h"
#include "Shader.h"
#include "Particle.h"
#include "Camera.h"


class ParticleRenderer
{
public:
	ParticleRenderer();
	void CleanUp();
	void Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, Camera* camera);
	void RenderBefore();
	void RenderAfter();
	void RenderInstanced(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, Camera* camera);
	void UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix);
	void LoadTexCoordInfo(glm::vec2 texOffset1, glm::vec2 texOffset2, int numRows, float blendFactor);
	void LoadTexCoordInfoInstanced(glm::vec4 texOffsets, int numberOfRows);
	void BindTexture(ParticleTexture* particleTexture);
	~ParticleRenderer();

public:
	Quad* m_Quad;
	Shader* m_ShaderParticle;

	Quad* m_QuadInstanced;
	Shader* m_ShaderParticleInstanced;

};
