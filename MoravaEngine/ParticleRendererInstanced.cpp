#include "ParticleRendererInstanced.h"

#include "RendererBasic.h"
#include "Util.h"
#include "Log.h"


ParticleRendererInstanced::ParticleRendererInstanced()
{
	m_Shader = new Shader("Shaders/ThinMatrix/particle_instanced.vs", "Shaders/ThinMatrix/particle_instanced.fs");
	printf("ParticleRenderer: m_Shader compiled [programID=%d]\n", m_Shader->GetProgramID());

	m_Mesh = new QuadInstanced();

	static_cast<QuadInstanced*>(m_Mesh)->CreateEmptyVBO(INSTANCE_DATA_LENGTH * MAX_INSTANCES);

	// model-view matrix in attribute slots 1 to 4
	static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(1, 4, INSTANCE_DATA_LENGTH,  0);
	static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(2, 4, INSTANCE_DATA_LENGTH,  4);
	static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(3, 4, INSTANCE_DATA_LENGTH,  8);
	static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(4, 4, INSTANCE_DATA_LENGTH, 12);
	// texture offsets in attribute slot 5
	static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(5, 4, INSTANCE_DATA_LENGTH, 16);
	// blend factor in attribute slot 6
	static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(6, 1, INSTANCE_DATA_LENGTH, 20);

	printf("ParticleRendererInstanced::ParticleRendererInstanced m_VAO = %i\n",           static_cast<QuadInstanced*>(m_Mesh)->GetVAO());
	printf("ParticleRendererInstanced::ParticleRendererInstanced m_VBO = %i\n",           static_cast<QuadInstanced*>(m_Mesh)->GetVBO());
	printf("ParticleRendererInstanced::ParticleRendererInstanced m_VBO_Instanced = %i\n", static_cast<QuadInstanced*>(m_Mesh)->GetVBOInstanced());
}

void ParticleRendererInstanced::RenderBefore()
{
	// glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE
	glDepthMask(GL_FALSE);
}

void ParticleRendererInstanced::RenderAfter()
{
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void ParticleRendererInstanced::Render(std::map<ParticleTexture*, std::vector<Particle*>*>* particleMap, glm::mat4 viewMatrix)
{
	RenderBefore();

	for (auto it_map = particleMap->begin(); it_map != particleMap->end(); it_map++)
	{
		ParticleTexture* particleTexture = it_map->first;

		m_Pointer = 0;
		std::vector<Particle*> particleVector = *it_map->second;
		unsigned int floatCount = (unsigned int)particleVector.size() * INSTANCE_DATA_LENGTH;
		m_VBO_Data = new float[floatCount];

		for (auto particle : particleVector)
		{
			UpdateModelViewMatrix(particle->GetPosition(), particle->GetRotation(), particle->GetScale(), viewMatrix, m_VBO_Data);
			LoadTexCoordInfo(particle, particleTexture->GetNumberOfRows(), m_VBO_Data);
			// m_Mesh->Render();
		}

		static_cast<QuadInstanced*>(m_Mesh)->UpdateVBO(floatCount, m_VBO_Data);

		m_Shader->Bind();
		BindTexture(particleTexture);
		m_Shader->setMat4("projection", RendererBasic::GetProjectionMatrix());
		m_Shader->setInt("albedoMap", 0);
		m_Shader->setFloat("numberOfRows", (float)particleTexture->GetNumberOfRows());

		static_cast<QuadInstanced*>(m_Mesh)->Render((unsigned int)particleVector.size());

		//	printf("\nm_VBO_Data\n");
		//	for (unsigned int i = 0; i < floatCount; i++)
		//		printf("[%i] %.2ff ", i, m_VBO_Data[i]);
	}

	RenderAfter();

	m_Shader->Unbind();
}

void ParticleRendererInstanced::BindTexture(ParticleTexture* particleTexture)
{
	// printf("ParticleRendererInstanced::BindTexture GetTextureID = %i\n", particleTexture->GetTextureID());
	particleTexture->Bind(0);
}

void ParticleRendererInstanced::UpdateModelViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::mat4 viewMatrix, float* vboData)
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

	// m_Shader->Bind();
	// m_Shader->setMat4("modelView", modelViewMatrix);
}

void ParticleRendererInstanced::StoreMatrixData(glm::mat4 matrix, float* vboData)
{
	vboData[m_Pointer++] = matrix[0][0];
	vboData[m_Pointer++] = matrix[0][1];
	vboData[m_Pointer++] = matrix[0][2];
	vboData[m_Pointer++] = matrix[0][3];

	vboData[m_Pointer++] = matrix[1][0];
	vboData[m_Pointer++] = matrix[1][1];
	vboData[m_Pointer++] = matrix[1][2];
	vboData[m_Pointer++] = matrix[1][3];

	vboData[m_Pointer++] = matrix[2][0];
	vboData[m_Pointer++] = matrix[2][1];
	vboData[m_Pointer++] = matrix[2][2];
	vboData[m_Pointer++] = matrix[2][3];

	vboData[m_Pointer++] = matrix[3][0];
	vboData[m_Pointer++] = matrix[3][1];
	vboData[m_Pointer++] = matrix[3][2];
	vboData[m_Pointer++] = matrix[3][3];
}

void ParticleRendererInstanced::LoadTexCoordInfo(Particle* particle, int numberOfRows, float* vboData)
{
	vboData[m_Pointer++] = particle->GetTexOffset1().x;
	vboData[m_Pointer++] = particle->GetTexOffset1().y;
	vboData[m_Pointer++] = particle->GetTexOffset2().x;
	vboData[m_Pointer++] = particle->GetTexOffset2().y;
	vboData[m_Pointer++] = particle->GetBlend();

	// TODO - remove old uniforms
	// m_Shader->setVec4("texOffsets", glm::vec4(particle->GetTexOffset1(), particle->GetTexOffset2()));
	// m_Shader->setFloat("blendFactor", particle->GetBlend());
}

void ParticleRendererInstanced::CleanUp()
{
	// delete m_VBO_Data;
	delete m_Mesh;
	delete m_Shader;
}

ParticleRendererInstanced::~ParticleRendererInstanced()
{
	CleanUp();
}
