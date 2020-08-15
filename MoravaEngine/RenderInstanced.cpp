#include "RenderInstanced.h"

#include "CommonValues.h"
#include "Log.h"


RenderInstanced::RenderInstanced()
{
	m_VBO_Instanced = -1;
	m_InstanceCount = -1;
}

RenderInstanced::RenderInstanced(TerrainBase* terrain, Texture* texture, Mesh* mesh)
{
	m_Terrain = (TerrainVoxel*)terrain;
	m_Texture = texture;
	m_Mesh = mesh;

	m_ModelMatrix = glm::mat4(1.0f);
	m_InstanceColor = glm::vec4(1.0f);

	m_IntersectPosition = glm::ivec3(0);
	m_DeleteMode = nullptr;

	m_VBO_Instanced = -1;
	m_InstanceCount = -1;

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

	int voxelIndex = 0;
	for (auto it = m_Terrain->m_Voxels.begin(); it != m_Terrain->m_Voxels.end(); it++)
	{
		m_ModelMatrix = glm::mat4(1.0f);

		m_InstanceColor = (*it).second->color;

		if ((*it).second->position == m_IntersectPosition) {
			if (m_DeleteMode != nullptr && *m_DeleteMode == true)
				m_InstanceColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			else
				m_InstanceColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		}

		m_ModelMatrix = glm::translate(m_ModelMatrix, (glm::vec3)(*it).second->position);

		m_InstanceDataArray[voxelIndex].model = m_ModelMatrix;
		m_InstanceDataArray[voxelIndex].color = m_InstanceColor;
	
		voxelIndex++;
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
	if (m_InstanceCount != -1) {
		delete m_InstanceDataArray;
		m_InstanceCount = -1;
	}
	if (m_VBO_Instanced != -1) {
		glDeleteBuffers(1, &m_VBO_Instanced);
		m_VBO_Instanced = -1;
	}
}

void RenderInstanced::SetIntersectPosition(glm::ivec3 intersectPosition)
{
	m_IntersectPosition = intersectPosition;
}

void RenderInstanced::SetDeleteMode(bool* deleteMode)
{
	m_DeleteMode = deleteMode;
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
