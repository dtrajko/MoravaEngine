#include "RenderInstanced.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>


RenderInstanced::RenderInstanced()
{
}

RenderInstanced::RenderInstanced(Terrain3D* terrain, Texture* texture, Mesh* mesh)
{
	m_Terrain = terrain;
	m_Texture = texture;
	m_Mesh = mesh;

	m_ModelMatrix = glm::mat4(1.0f);
	m_InstanceColor = glm::vec4(1.0f);

	m_InstanceCount = m_Terrain->GetCellCount();
	m_InstanceDataArray = new InstanceData[m_InstanceCount];

	CreateVertexData(m_Terrain->m_Positions);
}

void RenderInstanced::CreateVertexData(std::vector<glm::vec3> positions)
{
	CreateDataStructure(positions);
	CreateVertexArray();
}

void RenderInstanced::CreateDataStructure(std::vector<glm::vec3> positions)
{
	for (unsigned int i = 0; i < positions.size(); i++)
	{
		float color = positions[i].y / m_Terrain->m_Scale.y;
		m_ModelMatrix = glm::mat4(1.0f);
		m_InstanceColor = glm::vec4(color, color, 1.0f, 1.0f);
		m_ModelMatrix = glm::translate(m_ModelMatrix, positions[i]);
		m_InstanceDataArray[i].model = m_ModelMatrix;
		m_InstanceDataArray[i].color = m_InstanceColor;
	}
}

void RenderInstanced::CreateVertexArray()
{
	m_BufferSize = m_InstanceCount * sizeof(InstanceData);

	// configure instanced array
	glGenBuffers(1, &m_VBO_Instanced);
	glBindVertexArray(m_Mesh->GetVAO());
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBufferData(GL_ARRAY_BUFFER, m_BufferSize, &m_InstanceDataArray[0], GL_STATIC_DRAW);

	unsigned int location = 3;

	// model-view matrix in attribute slots 1 to 4 / set attribute pointers for matrix (4 times vec4)
	glEnableVertexAttribArray(location + 0);
	glVertexAttribPointer(location + 0, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)0);
	glVertexAttribDivisor(location + 0, 1);

	glEnableVertexAttribArray(location + 1);
	glVertexAttribPointer(location + 1, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)16);
	glVertexAttribDivisor(location + 1, 1);

	glEnableVertexAttribArray(location + 2);
	glVertexAttribPointer(location + 2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)32);
	glVertexAttribDivisor(location + 2, 1);

	glEnableVertexAttribArray(location + 3);
	glVertexAttribPointer(location + 3, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)48);
	glVertexAttribDivisor(location + 3, 1);

	glEnableVertexAttribArray(location + 4);
	glVertexAttribPointer(location + 4, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (const void*)64);
	glVertexAttribDivisor(location + 4, 1);

	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderInstanced::Update()
{
}

RenderInstanced::~RenderInstanced()
{
	delete m_InstanceDataArray;
}

void RenderInstanced::Render()
{
	glBindVertexArray(m_Mesh->GetVAO());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Mesh->GetIBO());
	glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)m_Mesh->GetIndexCount(), GL_UNSIGNED_INT, 0, (GLsizei)m_InstanceCount);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);
}
