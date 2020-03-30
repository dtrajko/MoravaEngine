#include "Block.h"
#include "Vertex.h"

#include "GL/glew.h"



Block::Block()
	: Mesh()
{
}

Block::Block(float scaleX, float scaleY, float scaleZ, float txMultiplier)
{
	float sizeX = 0.5f * scaleX;
	float sizeY = 0.5f * scaleY;
	float sizeZ = 0.5f * scaleZ;

	float txCoX = 1.0f * scaleX / txMultiplier;
	float txCoY = 1.0f * scaleY / txMultiplier;
	float txCoZ = 1.0f * scaleZ / txMultiplier;

	float vertices[] = {
		//   X       Y       Z         U      V         NX     NY     NZ        TX     TY     TZ        BX     BY     BZ
		-sizeX,  sizeY, -sizeZ,    txCoX,  0.0f,     -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,
		-sizeX, -sizeY, -sizeZ,    txCoX, txCoY,     -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,
		 sizeX, -sizeY, -sizeZ,     0.0f, txCoY,      0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,
		 sizeX,  sizeY, -sizeZ,     0.0f,  0.0f,      0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,

		-sizeX,  sizeY,  sizeZ,     0.0f,  0.0f,     -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,
		-sizeX, -sizeY,  sizeZ,     0.0f, txCoY,     -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,
		 sizeX, -sizeY,  sizeZ,    txCoX, txCoY,      0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,
		 sizeX,  sizeY,  sizeZ,    txCoX,  0.0f,      0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,

		 sizeX,  sizeY, -sizeZ,    txCoZ,  0.0f,      0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,
		 sizeX, -sizeY, -sizeZ,    txCoZ, txCoY,      0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,
		 sizeX, -sizeY,  sizeZ,     0.0f, txCoY,      0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,
		 sizeX,  sizeY,  sizeZ,     0.0f,  0.0f,      0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,

		-sizeX,  sizeY, -sizeZ,     0.0f,  0.0f,     -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,
		-sizeX, -sizeY, -sizeZ,     0.0f, txCoY,     -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,
		-sizeX, -sizeY,  sizeZ,    txCoZ, txCoY,     -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,
		-sizeX,  sizeY,  sizeZ,    txCoZ,  0.0f,     -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,

		-sizeX,  sizeY,  sizeZ,     0.0f, txCoZ,     -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,
		-sizeX,  sizeY, -sizeZ,     0.0f,  0.0f,     -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,
		 sizeX,  sizeY, -sizeZ,    txCoX,  0.0f,      0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,
		 sizeX,  sizeY,  sizeZ,    txCoX, txCoZ,      0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,

		-sizeX, -sizeY,  sizeZ,     0.0f, txCoZ,     -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,
		-sizeX, -sizeY, -sizeZ,     0.0f,  0.0f,     -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,
		 sizeX, -sizeY, -sizeZ,    txCoX,  0.0f,      0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,
		 sizeX, -sizeY,  sizeZ,    txCoX, txCoZ,      0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,
	};

	unsigned int vertexCount = 14 * 4 * 6;

	unsigned int indices[] =
	{
		 0,  3,  1,
		 3,  2,  1,
		 4,  5,  7,
		 7,  5,  6,
		 8, 11,  9,
		11, 10,  9,
		12, 13, 15,
		15, 13, 14,
		16, 19, 17,
		19, 18, 17,
		20, 21, 23,
		23, 21, 22,
	};

	m_IndexCount = 6 * 6;

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Position));
	// tex coord
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexCoord));
	// normal
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Normal));
	// tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Tangent));
	// bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Bitangent));

	glBindBuffer(GL_ARRAY_BUFFER, 0);         // Unbind VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

void Block::Render()
{
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

Block::~Block()
{
	if (m_IBO != 0)
	{
		glDeleteBuffers(1, &m_IBO);
		m_IBO = 0;
	}
	if (m_VBO != 0)
	{
		glDeleteBuffers(1, &m_VBO);
		m_VBO = 0;
	}
	if (m_VAO != 0)
	{
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
	}
	m_IndexCount = 0;

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);
}
