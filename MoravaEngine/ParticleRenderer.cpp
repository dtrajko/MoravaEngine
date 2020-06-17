#include "ParticleRenderer.h"

#include "RendererBasic.h"


ParticleRenderer::ParticleRenderer()
{
	m_ShaderParticle = new Shader("Shaders/ThinMatrix/particle.vs", "Shaders/ThinMatrix/particle.fs");
	printf("ParticleRenderer: m_ShaderParticle compiled [programID=%d]\n", m_ShaderParticle->GetProgramID());

	m_Quad = new Quad();
}

void ParticleRenderer::RenderBefore()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE
	glDepthMask(GL_FALSE);
}

void ParticleRenderer::RenderAfter()
{
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void ParticleRenderer::Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, Camera* camera)
{
	glm::mat4 viewMatrix = camera->CalculateViewMatrix();
	m_ShaderParticle->Bind();
	m_ShaderParticle->setMat4("projection", RendererBasic::GetProjectionMatrix());
	m_ShaderParticle->setInt("albedoMap", 0);

	RenderBefore();

	for (auto it_map = particleMap->begin(); it_map != particleMap->end(); it_map++)
	{
		BindTexture(it_map->first);

		for (auto particle : *it_map->second)
		{
			UpdateModelViewMatrix(particle->GetPosition(), particle->GetRotation(), particle->GetScale(), viewMatrix);
			LoadTexCoordInfo(particle->GetTexOffset1(), particle->GetTexOffset2(), particle->GetTexture()->GetNumberOfRows(), particle->GetBlend());
			m_Quad->Render();
		}
	}

	RenderAfter();

	m_ShaderParticle->Unbind();
}

void ParticleRenderer::BindTexture(ParticleTexture* particleTexture)
{
	particleTexture->Bind(0);
}

void ParticleRenderer::UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix)
{
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);

	modelMatrix[0][0] = viewMatrix[0][0];
	modelMatrix[0][1] = viewMatrix[1][0];
	modelMatrix[0][2] = viewMatrix[2][0];

	modelMatrix[1][0] = viewMatrix[0][1];
	modelMatrix[1][1] = viewMatrix[1][1];
	modelMatrix[1][2] = viewMatrix[2][1];

	modelMatrix[2][0] = viewMatrix[0][2];
	modelMatrix[2][1] = viewMatrix[1][2];
	modelMatrix[2][2] = viewMatrix[2][2];

	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
	modelMatrix = glm::scale(modelMatrix, scale);

	glm::mat4 modelViewMatrix = viewMatrix * modelMatrix;

	m_ShaderParticle->setMat4("modelView", modelViewMatrix);
}

void ParticleRenderer::LoadTexCoordInfo(glm::vec2 texOffset1, glm::vec2 texOffset2, int numRows, float blendFactor)
{
	m_ShaderParticle->setVec2("texOffset1", texOffset1);
	m_ShaderParticle->setVec2("texOffset2", texOffset2);
	m_ShaderParticle->setFloat("texCoordInfo.numRows", (float)numRows);
	m_ShaderParticle->setFloat("texCoordInfo.blendFactor", blendFactor);
}

void ParticleRenderer::CleanUp()
{
	delete m_Quad;
	delete m_ShaderParticle;
}

ParticleRenderer::~ParticleRenderer()
{
	CleanUp();
}
