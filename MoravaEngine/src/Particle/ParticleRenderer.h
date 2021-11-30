#pragma once

#include "Camera/CameraController.h"
#include "Mesh/Quad.h"
#include "Particle/Particle.h"
#include "Shader/MoravaShader.h"


class Particle;

class ParticleRenderer
{
public:
	ParticleRenderer();
	virtual void Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, glm::mat4 viewMatrix);
	virtual void RenderBefore();
	virtual void RenderAfter();
	void UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix);
	void LoadTexCoordInfo(glm::vec2 texOffset1, glm::vec2 texOffset2, int numRows, float blendFactor);
	virtual void BindTexture(ParticleTexture* particleTexture);
	virtual void CleanUp();
	~ParticleRenderer();

protected:
	Mesh* m_Mesh;
	H2M::Ref<MoravaShader> m_Shader;

};
