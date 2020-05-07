#include "Block.h"
#include "VertexTiling.h"

#include "GL/glew.h"


Block::Block()
	: Mesh()
{
}

Block::Block(glm::vec3 scale)
{
	m_Scale = scale;
	Generate(scale);
}

void Block::Update(glm::vec3 scale)
{
	if (scale != m_Scale)
		Generate(scale);
}

void Block::Generate(glm::vec3 scale)
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
		//   X       Y       Z         U      V         NX     NY     NZ        TX     TY     TZ        BX     BY     BZ
		-sizeX,  sizeY, -sizeZ,    txCoX,  0.0f,     -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    tilingFactor,
		-sizeX, -sizeY, -sizeZ,    txCoX, txCoY,     -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    tilingFactor,
		 sizeX, -sizeY, -sizeZ,     0.0f, txCoY,      0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,    tilingFactor,
		 sizeX,  sizeY, -sizeZ,     0.0f,  0.0f,      0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,    tilingFactor,

		-sizeX,  sizeY,  sizeZ,     0.0f,  0.0f,     -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    tilingFactor,
		-sizeX, -sizeY,  sizeZ,     0.0f, txCoY,     -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    tilingFactor,
		 sizeX, -sizeY,  sizeZ,    txCoX, txCoY,      0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,    tilingFactor,
		 sizeX,  sizeY,  sizeZ,    txCoX,  0.0f,      0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,    tilingFactor,

		 sizeX,  sizeY, -sizeZ,    txCoZ,  0.0f,      0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,    tilingFactor,
		 sizeX, -sizeY, -sizeZ,    txCoZ, txCoY,      0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,    tilingFactor,
		 sizeX, -sizeY,  sizeZ,     0.0f, txCoY,      0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,    tilingFactor,
		 sizeX,  sizeY,  sizeZ,     0.0f,  0.0f,      0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,    tilingFactor,

		-sizeX,  sizeY, -sizeZ,     0.0f,  0.0f,     -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    tilingFactor,
		-sizeX, -sizeY, -sizeZ,     0.0f, txCoY,     -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    tilingFactor,
		-sizeX, -sizeY,  sizeZ,    txCoZ, txCoY,     -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    tilingFactor,
		-sizeX,  sizeY,  sizeZ,    txCoZ,  0.0f,     -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    tilingFactor,

		-sizeX,  sizeY,  sizeZ,     0.0f, txCoZ,     -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    tilingFactor,
		-sizeX,  sizeY, -sizeZ,     0.0f,  0.0f,     -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    tilingFactor,
		 sizeX,  sizeY, -sizeZ,    txCoX,  0.0f,      0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,    tilingFactor,
		 sizeX,  sizeY,  sizeZ,    txCoX, txCoZ,      0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,    tilingFactor,

		-sizeX, -sizeY,  sizeZ,     0.0f, txCoZ,     -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    tilingFactor,
		-sizeX, -sizeY, -sizeZ,     0.0f,  0.0f,     -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    tilingFactor,
		 sizeX, -sizeY, -sizeZ,    txCoX,  0.0f,      0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,    tilingFactor,
		 sizeX, -sizeY,  sizeZ,    txCoX, txCoZ,      0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,    tilingFactor,
	};

	unsigned int vertexCount = 15 * 4 * 6;

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTiling), (const void*)offsetof(VertexTiling, Position));
	// tex coord
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTiling), (const void*)offsetof(VertexTiling, TexCoord));
	// normal
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTiling), (const void*)offsetof(VertexTiling, Normal));
	// tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTiling), (const void*)offsetof(VertexTiling, Tangent));
	// bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTiling), (const void*)offsetof(VertexTiling, Bitangent));
	// tilingFactor
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(VertexTiling), (const void*)offsetof(VertexTiling, TilingFactor));

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
	glDisableVertexAttribArray(5);
}
