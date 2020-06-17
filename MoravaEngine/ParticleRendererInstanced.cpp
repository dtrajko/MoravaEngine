#include "ParticleRendererInstanced.h"

#include "RendererBasic.h"


ParticleRendererInstanced::ParticleRendererInstanced()
{
	m_ShaderParticleInstanced = new Shader("Shaders/ThinMatrix/particle_instanced.vs", "Shaders/ThinMatrix/particle_instanced.fs");
	printf("ParticleRenderer: m_ShaderParticleInstanced compiled [programID=%d]\n", m_ShaderParticleInstanced->GetProgramID());

	m_QuadInstanced = new QuadInstanced();

	m_VBO_Data = new std::vector<float>();

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
		std::vector<Particle*> particleVector = *it_map->second;
		m_VBO_Data->resize(particleVector.size() * INSTANCE_DATA_LENGTH);

		for (auto particle : particleVector)
		{
			UpdateModelViewMatrix(particle->GetPosition(), particle->GetRotation(), particle->GetScale(), viewMatrix, m_VBO_Data);
			UpdateTexCoordInfo(particle, m_VBO_Data);
		}

		m_QuadInstanced->UpdateVBO(m_VBO_Instanced, (unsigned int)m_VBO_Data->size(), m_VBO_Data);
		m_QuadInstanced->Render(particleVector.size());
	}

	RenderAfter();

	m_ShaderParticleInstanced->Unbind();
}

void ParticleRendererInstanced::BindTexture(ParticleTexture* particleTexture)
{
	particleTexture->Bind(0);
}

void ParticleRendererInstanced::UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix, std::vector<float>* vboData)
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

	StoreMatrixData(modelViewMatrix, vboData);

	m_ShaderParticleInstanced->setMat4("modelView", modelViewMatrix);
}

void ParticleRendererInstanced::StoreMatrixData(glm::mat4 matrix, std::vector<float>* vboData)
{
	vboData->at(m_Pointer++) = matrix[0][0];
	vboData->at(m_Pointer++) = matrix[0][1];
	vboData->at(m_Pointer++) = matrix[0][2];
	vboData->at(m_Pointer++) = matrix[0][3];

	vboData->at(m_Pointer++) = matrix[1][0];
	vboData->at(m_Pointer++) = matrix[1][1];
	vboData->at(m_Pointer++) = matrix[1][2];
	vboData->at(m_Pointer++) = matrix[1][3];

	vboData->at(m_Pointer++) = matrix[2][0];
	vboData->at(m_Pointer++) = matrix[2][1];
	vboData->at(m_Pointer++) = matrix[2][2];
	vboData->at(m_Pointer++) = matrix[2][3];

	vboData->at(m_Pointer++) = matrix[3][0];
	vboData->at(m_Pointer++) = matrix[3][1];
	vboData->at(m_Pointer++) = matrix[3][2];
	vboData->at(m_Pointer++) = matrix[3][3];
}

void ParticleRendererInstanced::UpdateTexCoordInfo(Particle* particle, std::vector<float>* vboData)
{
	vboData->at(m_Pointer++) = particle->GetTexOffset1().x;
	vboData->at(m_Pointer++) = particle->GetTexOffset1().y;
	vboData->at(m_Pointer++) = particle->GetTexOffset2().x;
	vboData->at(m_Pointer++) = particle->GetTexOffset2().y;
	vboData->at(m_Pointer++) = particle->GetBlend();

	m_ShaderParticleInstanced->setFloat("numberOfRows", (float)particle->GetTexture()->GetNumberOfRows());
}

void ParticleRendererInstanced::CleanUp()
{
	delete m_VBO_Data;
	delete m_QuadInstanced;
	delete m_ShaderParticleInstanced;
}

ParticleRendererInstanced::~ParticleRendererInstanced()
{
	CleanUp();
}
