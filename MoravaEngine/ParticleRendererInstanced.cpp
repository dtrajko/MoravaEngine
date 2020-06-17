#include "ParticleRendererInstanced.h"

#include "RendererBasic.h"


ParticleRendererInstanced::ParticleRendererInstanced()
{
	m_ShaderParticleInstanced = new Shader("Shaders/ThinMatrix/particle_instanced.vs", "Shaders/ThinMatrix/particle_instanced.fs");
	printf("ParticleRenderer: m_ShaderParticleInstanced compiled [programID=%d]\n", m_ShaderParticleInstanced->GetProgramID());

	m_QuadInstanced = new QuadInstanced();

	m_VBO_Instanced = m_QuadInstanced->CreateEmptyVBO(INSTANCE_DATA_LENGTH * MAX_INSTANCES);
	m_QuadInstanced->LoadToVAO();
	m_QuadInstanced->AddInstancedAttribute(m_QuadInstanced->GetVAO(), m_VBO_Instanced, 1, 4, INSTANCE_DATA_LENGTH,  0);
	m_QuadInstanced->AddInstancedAttribute(m_QuadInstanced->GetVAO(), m_VBO_Instanced, 2, 4, INSTANCE_DATA_LENGTH,  4);
	m_QuadInstanced->AddInstancedAttribute(m_QuadInstanced->GetVAO(), m_VBO_Instanced, 3, 4, INSTANCE_DATA_LENGTH,  8);
	m_QuadInstanced->AddInstancedAttribute(m_QuadInstanced->GetVAO(), m_VBO_Instanced, 4, 4, INSTANCE_DATA_LENGTH, 12);
	m_QuadInstanced->AddInstancedAttribute(m_QuadInstanced->GetVAO(), m_VBO_Instanced, 5, 4, INSTANCE_DATA_LENGTH, 16);
	m_QuadInstanced->AddInstancedAttribute(m_QuadInstanced->GetVAO(), m_VBO_Instanced, 6, 1, INSTANCE_DATA_LENGTH, 20);

	glBindAttribLocation(m_ShaderParticleInstanced->GetProgramID(), 0, "aPosition");
	glBindAttribLocation(m_ShaderParticleInstanced->GetProgramID(), 1, "aTexCoord");
	glBindAttribLocation(m_ShaderParticleInstanced->GetProgramID(), 2, "modelView");
	glBindAttribLocation(m_ShaderParticleInstanced->GetProgramID(), 6, "texOffsets");
	glBindAttribLocation(m_ShaderParticleInstanced->GetProgramID(), 7, "blendFactor");
}

void ParticleRendererInstanced::RenderBefore()
{
	m_ShaderParticleInstanced->Bind();
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	glEnableVertexAttribArray(7);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE
	glDepthMask(GL_FALSE);
}

void ParticleRendererInstanced::RenderAfter()
{
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(5);
	glDisableVertexAttribArray(6);
	glDisableVertexAttribArray(7);
}

void ParticleRendererInstanced::Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, Camera* camera)
{
	glm::mat4 viewMatrix = camera->CalculateViewMatrix();
	m_ShaderParticleInstanced->Bind();
	m_ShaderParticleInstanced->setMat4("projection", RendererBasic::GetProjectionMatrix());
	m_ShaderParticleInstanced->setInt("albedoMap", 0);

	RenderBefore();

	for (auto it_map = particleMap->begin(); it_map != particleMap->end(); it_map++)
	{
		BindTexture(it_map->first);
		m_Pointer = 0;
		std::vector<Particle*> particleList = *it_map->second;
		for (auto particle : particleList)
		{
			m_VBO_Data = new std::vector<float>();
			m_VBO_Data->resize(particleList.size() * INSTANCE_DATA_LENGTH);
			UpdateModelViewMatrix(particle->GetPosition(), particle->GetRotation(), particle->GetScale(), viewMatrix);
			glm::vec4 texOffsets = glm::vec4(0.0f); // TODO TexOffsets data
			LoadTexCoordInfo(texOffsets, particle->GetTexture()->GetNumberOfRows());
			m_QuadInstanced->Render();
		}
	}

	RenderAfter();

	m_ShaderParticleInstanced->Unbind();
}

void ParticleRendererInstanced::BindTexture(ParticleTexture* particleTexture)
{
	particleTexture->Bind(0);
}

void ParticleRendererInstanced::UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix)
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

	m_ShaderParticleInstanced->setMat4("modelView", modelViewMatrix);
}

void ParticleRendererInstanced::LoadTexCoordInfo(glm::vec4 texOffsets, int numberOfRows)
{
	m_ShaderParticleInstanced->setFloat("numberOfRows", (float)numberOfRows);
}

void ParticleRendererInstanced::CleanUp()
{
	delete m_QuadInstanced;
	delete m_ShaderParticleInstanced;
}

ParticleRendererInstanced::~ParticleRendererInstanced()
{
	CleanUp();
}
