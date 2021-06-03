#include "Particle/ParticleRenderer.h"

#include "Core/Util.h"
#include "Renderer/RendererBasic.h"


ParticleRenderer::ParticleRenderer()
{
	m_Shader = new MoravaShader("Shaders/ThinMatrix/particle.vs", "Shaders/ThinMatrix/particle.fs");
	printf("ParticleRenderer: m_Shader compiled [programID=%d]\n", m_Shader->GetProgramID());

	m_Mesh = new Quad();
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

void ParticleRenderer::Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, glm::mat4 viewMatrix)
{
	m_Shader->Bind();
	m_Shader->SetMat4("projection", RendererBasic::GetProjectionMatrix());
	m_Shader->SetInt("albedoMap", 0);

	RenderBefore();

	for (auto it_map = particleMap->begin(); it_map != particleMap->end(); it_map++)
	{
		ParticleTexture* particleTexture = it_map->first;
		BindTexture(particleTexture);

		for (auto particle : *it_map->second)
		{
			UpdateModelViewMatrix(particle->GetPosition(), particle->GetRotation(), particle->GetScale(), viewMatrix);
			LoadTexCoordInfo(particle->GetTexOffset1(), particle->GetTexOffset2(), particle->GetTexture()->GetNumberOfRows(), particle->GetBlend());
			m_Mesh->Render();
		}
	}

	RenderAfter();

	m_Shader->Unbind();
}

void ParticleRenderer::BindTexture(ParticleTexture* particleTexture)
{
	// printf("ParticleRenderer::BindTexture GetTextureID = %i\n", particleTexture->GetTextureID());
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

	m_Shader->SetMat4("modelView", modelViewMatrix);
}

void ParticleRenderer::LoadTexCoordInfo(glm::vec2 texOffset1, glm::vec2 texOffset2, int numRows, float blendFactor)
{
	m_Shader->setVec2("texOffset1", texOffset1);
	m_Shader->setVec2("texOffset2", texOffset2);
	m_Shader->SetFloat("texCoordInfo.numRows", (float)numRows);
	m_Shader->SetFloat("texCoordInfo.blendFactor", blendFactor);
}

void ParticleRenderer::CleanUp()
{
	delete m_Mesh;
	delete m_Shader;
}

ParticleRenderer::~ParticleRenderer()
{
	CleanUp();
}
