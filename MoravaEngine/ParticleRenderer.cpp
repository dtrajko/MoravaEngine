#include "ParticleRenderer.h"


ParticleRenderer::ParticleRenderer()
{
	m_Quad = new Quad();

	m_ShaderParticle = new Shader("Shaders/ThinMatrix/particle.vs", "Shaders/ThinMatrix/particle.fs");
	printf("ParticleRenderer: m_ShaderParticle compiled [programID=%d]\n", m_ShaderParticle->GetProgramID());
}

ParticleRenderer::ParticleRenderer(glm::mat4 projectionMatrix)
	: ParticleRenderer()
{
	m_ShaderParticle->Bind();
	m_ShaderParticle->setMat4("projection", projectionMatrix);
}

void ParticleRenderer::CleanUp()
{
	delete m_Quad;
	delete m_ShaderParticle;
}

void ParticleRenderer::Render(std::vector<Particle*>* particles, Camera* camera)
{
	glm::mat4 viewMatrix = camera->CalculateViewMatrix();
	m_ShaderParticle->Bind();
	m_ShaderParticle->setMat4("view", viewMatrix);

	/**** Begin RenderBegin ****/
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	/**** End RenderBegin ****/

	std::vector<Particle*>::iterator it;
	for (it = particles->begin(); it != particles->end(); ++it)
	{
		UpdateModelViewMatrix((*it)->GetPosition(), (*it)->GetRotation(), (*it)->GetScale(), viewMatrix);
		m_Quad->Render();
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

ParticleRenderer::~ParticleRenderer()
{
	CleanUp();
}
