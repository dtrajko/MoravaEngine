#include "RenderInstanced.h"

#include <glm/gtc/matrix_transform.hpp>

#include <GL/glew.h>


RenderInstanced::RenderInstanced()
{
}

RenderInstanced::RenderInstanced(unsigned int instanceCount)
{
	m_ModelMatrix = glm::mat4(1.0f);
	m_InstanceCount = instanceCount;
	m_ModelMatrices = new glm::mat4[m_InstanceCount];
}

void RenderInstanced::SetMesh(Texture* texture, Mesh* mesh)
{
	m_Texture = texture;
	m_Mesh = mesh;
	// TODO: improve it to work with multiple Textures and multiple Meshes
}

void RenderInstanced::CreateVertexAttributes(std::vector<glm::vec3> positions)
{
	// Based on LearnOpenGL Asteroids / ThinMatrix Particle System

	for (unsigned int i = 0; i < positions.size(); i++)
	{
		m_ModelMatrix = glm::mat4(1.0f);
		m_ModelMatrix = glm::translate(m_ModelMatrix, positions[i]);
		m_ModelMatrices[i] = m_ModelMatrix;
	}

	unsigned int bufferSize = m_InstanceCount * sizeof(InstanceData);

	// configure instanced array
	glGenBuffers(1, &m_VBO_Instanced);
	glBindVertexArray(m_Mesh->GetVAO());
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBufferData(GL_ARRAY_BUFFER, bufferSize, &m_ModelMatrices[0], GL_STATIC_DRAW);

	// model-view matrix in attribute slots 1 to 4 / set attribute pointers for matrix (4 times vec4)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix), (const void*)offsetof(Matrix, Matrix::row_0));
	glVertexAttribDivisor(2, 1);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix), (const void*)offsetof(Matrix, Matrix::row_1));
	glVertexAttribDivisor(3, 1);

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix), (const void*)offsetof(Matrix, Matrix::row_2));
	glVertexAttribDivisor(4, 1);

	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix), (const void*)offsetof(Matrix, Matrix::row_3));
	glVertexAttribDivisor(5, 1);

	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

RenderInstanced::~RenderInstanced()
{
	delete m_ModelMatrices;
}

void RenderInstanced::Render()
{
	glBindVertexArray(m_Mesh->GetVAO());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Mesh->GetIBO());
	glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)m_Mesh->GetIndexCount(), GL_UNSIGNED_INT, 0, (GLsizei)m_InstanceCount);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);
}
