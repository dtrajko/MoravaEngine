#include "RenderInstanced.h"

#include <glm/gtc/matrix_transform.hpp>

#include <GL/glew.h>


RenderInstanced::RenderInstanced()
{
	m_ModelMatrix = glm::mat4(1.0f);
	m_ModelMatrices = new glm::mat4[m_InstanceCount]; // asteroids
}

void RenderInstanced::CreateVertexAttributes(std::vector<glm::vec3> positions)
{
	// Based on LearnOpenGL Asteroids / ThinMatrix Particle System

	for (unsigned int i = 0; i < positions.size(); i++)
	{
		m_ModelMatrix = glm::translate(m_ModelMatrix, positions[i]);
		m_ModelMatrices[i] = m_ModelMatrix;
	}

	unsigned int bufferSize = m_InstanceCount * sizeof(InstanceData);

	// configure instanced array
	glGenBuffers(1, &m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBufferData(GL_ARRAY_BUFFER, bufferSize, &m_ModelMatrices[0], GL_STATIC_DRAW);

	// model-view matrix in attribute slots 1 to 4 / set attribute pointers for matrix (4 times vec4)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, sizeof(Matrix::row_0), GL_FLOAT, GL_FALSE, sizeof(Matrix), (const void*)offsetof(Matrix, Matrix::row_0));
	glVertexAttribDivisor(2, 1);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, sizeof(Matrix::row_1), GL_FLOAT, GL_FALSE, sizeof(Matrix), (const void*)offsetof(Matrix, Matrix::row_1));
	glVertexAttribDivisor(3, 1);

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, sizeof(Matrix::row_2), GL_FLOAT, GL_FALSE, sizeof(Matrix), (const void*)offsetof(Matrix, Matrix::row_2));
	glVertexAttribDivisor(4, 1);

	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, sizeof(Matrix::row_3), GL_FLOAT, GL_FALSE, sizeof(Matrix), (const void*)offsetof(Matrix, Matrix::row_3));
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
	size_t m_MeshIndicesSize = 0;
	size_t m_VertexCount = 0;
	size_t m_InstanceCount = 0;

	for (auto it_map = m_Map.begin(); it_map != m_Map.end(); it_map++)
	{
		Texture* texture = it_map->first;
		std::vector<Mesh*> meshVector = *it_map->second;

		for (auto mesh : meshVector)
		{

		}

		glBindVertexArray(m_VAO);
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)m_MeshIndicesSize, GL_UNSIGNED_INT, 0, (GLsizei)m_InstanceCount);
		// glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, (GLsizei)m_VertexCount, (GLsizei)m_InstanceCount); // alternative method
		glBindVertexArray(0);
	}
}
