#pragma once

#include "QuadInstanced.h"
#include "Shader.h"
#include "Particle.h"
#include "Camera.h"


class ParticleRendererInstanced
{
public:
	ParticleRendererInstanced();
	void Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, Camera* camera);
	void RenderBefore();
	void RenderAfter();
	void UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix);
	void LoadTexCoordInfo(glm::vec4 texOffsets, int numberOfRows);
	void BindTexture(ParticleTexture* particleTexture);
	void CleanUp();
	~ParticleRendererInstanced();

private:
	QuadInstanced* m_QuadInstanced;
	Shader* m_ShaderParticleInstanced;

	unsigned int m_VBO_Instanced; // a large VBO for instanced data

	int MAX_INSTANCES = 10000;
	int INSTANCE_DATA_LENGTH = 21; // 16F model view + 4F texOffsets + 1F blendFactor

	unsigned int m_Pointer;

	std::vector<float>* m_VBO_Data;

};
