#include "ParticleRenderer.h"

#include "RendererBasic.h"


ParticleRenderer::ParticleRenderer()
{
	m_ShaderParticle = new Shader("Shaders/ThinMatrix/particle.0.1.vs", "Shaders/ThinMatrix/particle.0.1.fs");
	printf("ParticleRenderer: m_ShaderParticle compiled [programID=%d]\n", m_ShaderParticle->GetProgramID());

	m_Quad = new Quad();
	// TextureLoader::Get()->GetTexture("Textures/ThinMatrix/lensFlare/tex3.png");
}

void ParticleRenderer::Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, Camera* camera)
{
	glm::mat4 viewMatrix = camera->CalculateViewMatrix();
	m_ShaderParticle->Bind();
	m_ShaderParticle->setMat4("projection", RendererBasic::GetProjectionMatrix());
	m_ShaderParticle->setInt("albedoMap", 0);

	/**** Begin RenderBegin ****/
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	/**** End RenderBegin ****/

	// printf("ParticleRenderer::Render particleMap.size = %zu\n", particleMap->size());

	std::map<ParticleTexture*, std::vector<Particle*>*>::iterator it_map;
	for (it_map = particleMap->begin(); it_map != particleMap->end(); it_map++)
	{
		ParticleTexture* particleTexture = it_map->first;
		std::vector<Particle*>* particleVec = it_map->second;

		// printf("ParticleRenderer::Render particleVec.size = %zu\n", particleVec.size());

		std::vector<Particle*>::iterator it_vec;
		particleTexture->Bind(0);
		for (it_vec = particleVec->begin(); it_vec != particleVec->end(); it_vec++)
		{
			UpdateModelViewMatrix((*it_vec)->GetPosition(), (*it_vec)->GetRotation(), (*it_vec)->GetScale(), viewMatrix);
			m_Quad->Render();
			// printf("ParticleRenderer is rendering a Quad ;P particleMap.size = %zu particleList.size = %zu\n", particleMap->size(), particleVec.size());
		}
	}

	/**** Begin RenderEnd ****/
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	/**** End RenderEnd ****/

	m_ShaderParticle->Unbind();
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

void ParticleRenderer::CleanUp()
{
	delete m_Quad;
	delete m_ShaderParticle;
}

ParticleRenderer::~ParticleRenderer()
{
	CleanUp();
}
