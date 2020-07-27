#include "RenderInstanced.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>


RenderInstanced::RenderInstanced()
{
}

RenderInstanced::RenderInstanced(TerrainBase* terrain, Texture* texture, Mesh* mesh)
{
	m_Terrain = (TerrainVoxel*)terrain;
	m_Texture = texture;
	m_Mesh = mesh;

	m_ModelMatrix = glm::mat4(1.0f);
	m_InstanceColor = glm::vec4(1.0f);

	m_IntersectPosition = nullptr;

	CreateVertexData();
}

void RenderInstanced::Update()
{
}

void RenderInstanced::CreateVertexData()
{
	Release();
	CreateDataStructure();
	CreateVertexArray();
}

void RenderInstanced::CreateDataStructure()
{
	m_InstanceCount = (unsigned int)m_Terrain->m_Voxels.size();
	m_InstanceDataArray = new InstanceData[m_InstanceCount];

	for (unsigned int i = 0; i < m_Terrain->m_Voxels.size(); i++)
	{
		m_ModelMatrix = glm::mat4(1.0f);

		float colorR = m_Terrain->m_Voxels[i].position.x / m_Terrain->m_Scale.x;
		float colorG = m_Terrain->m_Voxels[i].position.y / m_Terrain->m_Scale.y;
		float colorB = m_Terrain->m_Voxels[i].position.z / m_Terrain->m_Scale.z;

		// m_InstanceColor = glm::vec4(1.0f - colorR, colorG, 1.0f - colorB, 0.6f);
		m_InstanceColor = m_Terrain->m_Voxels[i].color;

		if (m_IntersectPosition != nullptr && m_Terrain->m_Voxels[i].position == *m_IntersectPosition) {
			m_InstanceColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		}

		m_ModelMatrix = glm::translate(m_ModelMatrix, m_Terrain->m_Voxels[i].position);

		if (i >= m_InstanceCount)
			printf("m_InstanceCount = %u, i = %u\n", m_InstanceCount, i);

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

void RenderInstanced::Release()
{
	delete m_InstanceDataArray;
	glDeleteBuffers(1, &m_VBO_Instanced);
}

void RenderInstanced::SetMouseCursorIntersectPosition(glm::vec3* intersectPosition)
{
	m_IntersectPosition = intersectPosition;
}

RenderInstanced::~RenderInstanced()
{
	Release();
}

void RenderInstanced::Render()
{
	glBindVertexArray(m_Mesh->GetVAO());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Mesh->GetIBO());
	glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)m_Mesh->GetIndexCount(), GL_UNSIGNED_INT, 0, (GLsizei)m_InstanceCount);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);
}
