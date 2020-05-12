#include "Cylinder.h"

#include <GL/glew.h>

#include "VertexTBN.h"


Cylinder::Cylinder() : Cylinder(glm::vec3(1.0f))
{
}

Cylinder::Cylinder(glm::vec3 scale) : Mesh()
{
	m_Scale = scale;

	m_CylinderSH = new CylinderSongHo(m_BaseRadius, m_TopRadius, m_Height, m_Sectors, m_Stacks, m_Smooth);

	Generate(m_Scale);
}

void Cylinder::Generate(glm::vec3 scale)
{
	m_Scale = scale;

	unsigned int vertexCountSH = m_CylinderSH->getVertexCount();
	unsigned int indexCountSH = m_CylinderSH->getIndexCount();
	unsigned int normalCountSH = m_CylinderSH->getNormalCount();
	unsigned int texCoordCountSH = m_CylinderSH->getTexCoordCount();

	m_Indices = m_CylinderSH->indices.data();

	unsigned int vertexStride = sizeof(VertexTBN) / sizeof(float);
	std::vector<float> vertices;

	for (unsigned int i = 0; i < vertexCountSH; i++)
	{
		// position
		vertices.push_back(m_CylinderSH->vertices.at(i * 3 + 0));
		vertices.push_back(m_CylinderSH->vertices.at(i * 3 + 1));
		vertices.push_back(m_CylinderSH->vertices.at(i * 3 + 2));

		// tex coords
		vertices.push_back(m_CylinderSH->texCoords.at(i * 2 + 0));
		vertices.push_back(m_CylinderSH->texCoords.at(i * 2 + 1));

		// normals
		vertices.push_back(m_CylinderSH->normals.at(i * 3 + 0));
		vertices.push_back(m_CylinderSH->normals.at(i * 3 + 1));
		vertices.push_back(m_CylinderSH->normals.at(i * 3 + 2));

		// tangents
		vertices.push_back(0.0f);
		vertices.push_back(0.0f);
		vertices.push_back(0.0f);

		// bitangents
		vertices.push_back(0.0f);
		vertices.push_back(0.0f);
		vertices.push_back(0.0f);
	};

	m_Vertices = &vertices[0];
	m_VertexCount = vertexCountSH;
	m_IndexCount = indexCountSH;

	// CalcAverageNormals(vertices.data(), m_VertexCount, cylinderSH.indices.data(), m_IndexCount);
	// CalcTangentSpace(vertices.data(), m_VertexCount, cylinderSH.indices.data(), m_IndexCount);

	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glGenBuffers(1, &m_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_CylinderSH->indices[0]) * m_CylinderSH->indices.size(), m_CylinderSH->indices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), (const void*)offsetof(VertexTBN, Position));
	// tex coord
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), (const void*)offsetof(VertexTBN, TexCoord));
	// normal
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), (const void*)offsetof(VertexTBN, Normal));
	// tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), (const void*)offsetof(VertexTBN, Tangent));
	// bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), (const void*)offsetof(VertexTBN, Bitangent));

	glBindBuffer(GL_ARRAY_BUFFER, 0);         // Unbind VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO

	vertices.clear();
}

void Cylinder::Update(glm::vec3 scale)
{
}

Cylinder::~Cylinder()
{
	delete m_CylinderSH;

	Clear();
}
