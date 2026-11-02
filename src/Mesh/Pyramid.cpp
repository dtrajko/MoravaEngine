#include "Mesh/Pyramid.h"

#include "Mesh/VertexTBN.h"

#include <GL/glew.h>


Pyramid::Pyramid() : Mesh()
{
	m_Scale = glm::vec3(1.0f);
}

Pyramid::Pyramid(glm::vec3 scale)
{
	m_Scale = scale;

	Generate(scale);
}

void Pyramid::Generate(glm::vec3 scale)
{
	m_Scale = scale;

	float sizeX = 0.5f * scale.x;
	float sizeY = 0.5f * scale.y;
	float sizeZ = 0.5f * scale.z;

	float txCoX = 1.0f * scale.x;
	float txCoY = 1.0f * scale.y;
	float txCoZ = 1.0f * scale.z;

	float tilingFactor = 0.5f;

	float vertices[] = {
		//   X       Y       Z         U          V         NX     NY     NZ        TX    TY    TZ        BX     BY     BZ
		// side vertices
		-sizeX, -sizeY,  sizeZ,     0.0f,      0.0f,      0.0f,  0.5f,  1.0f,     0.0f, 0.0f, 0.0f,     0.0f,  0.0f,  1.0f, // 0 front
		 sizeX, -sizeY,  sizeZ,    txCoX,      0.0f,      0.0f,  0.5f,  1.0f,     0.0f, 0.0f, 0.0f,     0.0f,  0.0f,  1.0f, // 1 front
		 sizeX, -sizeY,  sizeZ,     0.0f,      0.0f,      1.0f,  0.5f,  0.0f,     0.0f, 0.0f, 0.0f,     0.0f,  0.0f,  1.0f, // 2 right
		 sizeX, -sizeY, -sizeZ,    txCoX,      0.0f,      1.0f,  0.5f,  0.0f,     0.0f, 0.0f, 0.0f,     0.0f,  0.0f, -1.0f, // 3 right
		 sizeX, -sizeY, -sizeZ,     0.0f,      0.0f,      0.0f,  0.5f, -1.0f,     0.0f, 0.0f, 0.0f,     0.0f,  0.0f, -1.0f, // 4 back
		-sizeX, -sizeY, -sizeZ,    txCoX,      0.0f,      0.0f,  0.5f, -1.0f,     0.0f, 0.0f, 0.0f,     0.0f,  0.0f, -1.0f, // 5 back
		-sizeX, -sizeY, -sizeZ,     0.0f,      0.0f,     -1.0f,  0.5f,  0.0f,     0.0f, 0.0f, 0.0f,     0.0f,  0.0f, -1.0f, // 6 left
		-sizeX, -sizeY,  sizeZ,    txCoX,      0.0f,     -1.0f,  0.5f,  0.0f,     0.0f, 0.0f, 0.0f,     0.0f,  0.0f,  1.0f, // 7 left

		// top vertex
			 0,  sizeY,      0,    txCoX/2.0f, txCoY,     0.0f,  1.0f,  0.0f,     0.0f,  0.0f,  0.0f,   0.0f,  0.0f,  0.0f, // 8

	    // bottom vertices
	     sizeX, -sizeY,  sizeZ,     0.0f,       0.0f,     0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f,   0.0f,  0.0f,  0.0f, // 9  00
		-sizeX, -sizeY,  sizeZ,    txCoX,       0.0f,     0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f,   0.0f,  0.0f,  0.0f, // 10 10
		-sizeX, -sizeY, -sizeZ,    txCoX,      txCoY,     0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f,   0.0f,  0.0f,  0.0f, // 11 11
		 sizeX, -sizeY, -sizeZ,     0.0f,      txCoY,     0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f,   0.0f,  0.0f,  0.0f, // 12 01
	};

	unsigned int vertexCount = 14 * 13;

	unsigned int indices[] =
	{
		 0,   1,  8, // side front
		 2,   3,  8, // side right
		 4,   5,  8, // side back
		 6,   7,  8, // side left
		 9,  10, 12, // bottom
		 10, 11, 12, // bottom
	};

	m_IndexCount = 6 * 3;

	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glGenBuffers(1, &m_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * m_IndexCount, indices, GL_STATIC_DRAW);

	glGenBuffers(1, &m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertexCount, vertices, GL_STATIC_DRAW);

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

Pyramid::~Pyramid()
{
}
