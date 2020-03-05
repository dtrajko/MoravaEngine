#include "Tile2D.h"


Tile2D::Tile2D()
{
	VAO = 0;
	VBO = 0;
	IBO = 0;
	m_IndexCount = 0;
}

void Tile2D::CreateMesh(GLfloat* vertices, unsigned int* indices, unsigned int numOfVertices, unsigned int numOfIndices)
{
	m_IndexCount = numOfIndices;

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * numOfIndices, indices, GL_STATIC_DRAW);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * numOfVertices, vertices, GL_STATIC_DRAW);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (const void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);         // Unbind VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

void Tile2D::RenderMesh()
{
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

void Tile2D::ClearMesh()
{
	if (IBO != 0)
	{
		glDeleteBuffers(1, &IBO);
		IBO = 0;
	}
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
	m_IndexCount = 0;

	glDisableVertexAttribArray(0);
}

Tile2D::~Tile2D()
{
	ClearMesh();
}
