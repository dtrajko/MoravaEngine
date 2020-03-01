#include "Tile2D.h"


Tile2D::Tile2D()
{
	VAO = 0;
	VBO = 0;
}

void Tile2D::CreateMesh(GLfloat* vertices, unsigned int vertexCount)
{
	m_VertexCount = vertexCount;

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertexCount, vertices, GL_STATIC_DRAW);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (const void*)0);
	// tex coord
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (const void*)2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);         // Unbind VBO
	glBindVertexArray(0);                     // Unbind VAO
}

void Tile2D::RenderMesh()
{
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, m_VertexCount);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

void Tile2D::ClearMesh()
{
	if (VBO != 0)
	{
		glDeleteBuffers(1, &VBO);
		VBO = 0;
	}
	if (VAO != 0)
	{
		glDeleteVertexArrays(1, &VAO);
		VAO = 0;
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

Tile2D::~Tile2D()
{
	ClearMesh();
}
