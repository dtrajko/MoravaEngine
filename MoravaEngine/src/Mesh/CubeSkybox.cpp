#include "Mesh/CubeSkybox.h"

#include "Mesh/VertexTBN.h"

#include "GL/glew.h"

#include <vector>


CubeSkybox::CubeSkybox()
{
	std::vector<float> vertices {
		-1.0f,  1.0f, -1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, -1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, -1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, -1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
								    		       					    				     
		-1.0f,  1.0f,  1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f,  1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
	};

	m_Vertices = &vertices[0];
	m_VertexCount = (unsigned int)vertices.size();

	std::vector<unsigned int> indices = {
		// front
		0, 1, 2,
		2, 1, 3,
		// right
		2, 3, 5,
		5, 3, 7,
		// back
		5, 7, 4,
		4, 7, 6,
		// left
		4, 6, 0,
		0, 6, 1,
		// top
		4, 0, 5,
		5, 0, 2,
		// bottom
		1, 6, 3,
		3, 6, 7,
	};

	m_Indices = &indices[0];
	m_IndexCount = (unsigned int)indices.size();

	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glGenBuffers(1, &m_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_Indices[0]) * m_IndexCount, m_Indices, GL_STATIC_DRAW);

	glGenBuffers(1, &m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_Vertices[0]) * m_VertexCount, m_Vertices, GL_STATIC_DRAW);

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
}

CubeSkybox::~CubeSkybox()
{
}
