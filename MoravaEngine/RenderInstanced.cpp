#include "RenderInstanced.h"

#include <GL/glew.h>


RenderInstanced::RenderInstanced()
{
	m_ModelMatrices = new glm::mat4[m_InstanceCount]; // asteroids
	CreateVertexAttributesI();
}

void RenderInstanced::CreateVertexAttributesI()
{
	// based on ThinMatrix Particle System

	std::vector<Mesh*> meshVector;

	unsigned int floatCount = (unsigned int)meshVector.size() * INSTANCE_DATA_LENGTH;

	// CreateEmptyVBO(INSTANCE_DATA_LENGTH * m_MaxInstances);
	glGenBuffers(1, &m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), NULL, GL_STREAM_DRAW);

	unsigned int attribute;
	unsigned int dataSize;
	unsigned int instancedDataLength;
	unsigned int offset;

	// model-view matrix in attribute slots 1 to 4
	// static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(1, 4, INSTANCE_DATA_LENGTH, 0);
	attribute = 1;
	dataSize = 4;
	instancedDataLength = INSTANCE_DATA_LENGTH;
	offset = 0;

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glEnableVertexAttribArray(attribute);
	glVertexAttribPointer(attribute, dataSize, GL_FLOAT, GL_FALSE, instancedDataLength * sizeof(float), (const void*)(offset * sizeof(float)));
	glVertexAttribDivisor(attribute, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(2, 4, INSTANCE_DATA_LENGTH, 4);
	attribute = 2;
	dataSize = 4;
	instancedDataLength = INSTANCE_DATA_LENGTH;
	offset = 4;

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glEnableVertexAttribArray(attribute);
	glVertexAttribPointer(attribute, dataSize, GL_FLOAT, GL_FALSE, instancedDataLength * sizeof(float), (const void*)(offset * sizeof(float)));
	glVertexAttribDivisor(attribute, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(3, 4, INSTANCE_DATA_LENGTH, 8);
	attribute = 3;
	dataSize = 4;
	instancedDataLength = INSTANCE_DATA_LENGTH;
	offset = 8;

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glEnableVertexAttribArray(attribute);
	glVertexAttribPointer(attribute, dataSize, GL_FLOAT, GL_FALSE, instancedDataLength * sizeof(float), (const void*)(offset * sizeof(float)));
	glVertexAttribDivisor(attribute, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(4, 4, INSTANCE_DATA_LENGTH, 12);
	attribute = 4;
	dataSize = 4;
	instancedDataLength = INSTANCE_DATA_LENGTH;
	offset = 12;

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glEnableVertexAttribArray(attribute);
	glVertexAttribPointer(attribute, dataSize, GL_FLOAT, GL_FALSE, instancedDataLength * sizeof(float), (const void*)(offset * sizeof(float)));
	glVertexAttribDivisor(attribute, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// texture offsets in attribute slot 5
	// static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(5, 4, INSTANCE_DATA_LENGTH, 16);
	attribute = 5;
	dataSize = 4;
	instancedDataLength = INSTANCE_DATA_LENGTH;
	offset = 16;

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glEnableVertexAttribArray(attribute);
	glVertexAttribPointer(attribute, dataSize, GL_FLOAT, GL_FALSE, instancedDataLength * sizeof(float), (const void*)(offset * sizeof(float)));
	glVertexAttribDivisor(attribute, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// blend factor in attribute slot 6
	// static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(6, 1, INSTANCE_DATA_LENGTH, 20);
	attribute = 6;
	dataSize = 1;
	instancedDataLength = INSTANCE_DATA_LENGTH;
	offset = 20;

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glEnableVertexAttribArray(attribute);
	glVertexAttribPointer(attribute, dataSize, GL_FLOAT, GL_FALSE, instancedDataLength * sizeof(float), (const void*)(offset * sizeof(float)));
	glVertexAttribDivisor(attribute, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void RenderInstanced::CreateVertexAttributesII()
{
	// Based on LearnOpenGL Asteroids

	std::vector<Mesh*> m_Meshes;

	// configure instanced array
	// -------------------------
	unsigned int buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, m_InstanceCount * sizeof(glm::mat4), &m_ModelMatrices[0], GL_STATIC_DRAW);

	for (unsigned int i = 0; i < m_Meshes.size(); i++)
	{
		unsigned int VAO = m_Meshes[i]->GetVAO();
		glBindVertexArray(VAO);
		// set attribute pointers for matrix (4 times vec4)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);
	}
}

RenderInstanced::~RenderInstanced()
{
}

void RenderInstanced::Render()
{
	size_t m_MeshCount = 0;
	unsigned int m_VAO = 0;
	size_t m_MeshIndicesSize = 0;
	size_t m_InstancesCount = 1000;
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
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)m_MeshIndicesSize, GL_UNSIGNED_INT, 0, (GLsizei)m_InstancesCount);
		// glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, (GLsizei)m_VertexCount, (GLsizei)m_InstanceCount); // alternative method
		glBindVertexArray(0);
	}
}
