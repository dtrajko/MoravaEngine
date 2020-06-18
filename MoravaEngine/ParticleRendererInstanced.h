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
	virtual void Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, glm::mat4 viewMatrix) override;
	virtual void RenderBefore() override;
	virtual void RenderAfter() override;
	void UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix, std::vector<float>* vboData);
	void UpdateTexCoordInfo(Particle* particle, std::vector<float>* vboData);
	virtual void StoreMatrixData(glm::mat4 matrix, std::vector<float>* vboData);
	virtual void CleanUp() override;
	~ParticleRendererInstanced();

private:
	int MAX_INSTANCES = 10000;
	int INSTANCE_DATA_LENGTH = 21; // 16F model view + 4F texOffsets + 1F blendFactor

	unsigned int m_VBO_Instanced; // a large VBO for instanced data
	std::vector<float>* m_VBO_Data;
	unsigned int m_Pointer;

};
