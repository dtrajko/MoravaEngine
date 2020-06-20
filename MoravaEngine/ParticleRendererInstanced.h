#pragma once

#include "ParticleRenderer.h"
#include "QuadInstanced.h"
#include "Shader.h"
#include "Particle.h"
#include "Camera.h"


class ParticleRendererInstanced : public ParticleRenderer
{
public:
	ParticleRendererInstanced();
	ParticleRendererInstanced(int maxInstances);
	virtual void Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, glm::mat4 viewMatrix) override;
	virtual void RenderBefore() override;
	virtual void RenderAfter() override;
	void UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix, float* vboData);
	void LoadTexCoordInfo(Particle* particle, int numberOfRows, float* vboData);
	virtual void StoreMatrixData(glm::mat4 matrix, float* vboData);
	virtual void BindTexture(ParticleTexture* particleTexture) override;
	virtual void CleanUp() override;
	~ParticleRendererInstanced();

private:
	int m_MaxInstances;
	int INSTANCE_DATA_LENGTH = 21; // 16F model view + 4F texOffsets + 1F blendFactor

	float* m_VBO_Data;
	unsigned int m_Pointer;

};
