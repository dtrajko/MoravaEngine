#include "Sphere.h"

#include <cmath>

#include "Vertex.h"


Sphere::Sphere()
{
	VAO = 0;
	VBO = 0;
	IBO = 0;
	m_IndexCount = 0;
}

void Sphere::GenerateGeometry()
{
	const unsigned int X_SEGMENTS = 8;
	const unsigned int Y_SEGMENTS = 8;
	const float PI = 3.14159265359f;

	unsigned int vertexStride = (unsigned int)(sizeof(Vertex) / sizeof(float));
	m_VertexCount = sizeof(Vertex) * (X_SEGMENTS + 1) * (Y_SEGMENTS + 1);
	m_IndexCount = 2 * (X_SEGMENTS + 1) * Y_SEGMENTS;

	m_Vertices = new float[m_VertexCount];
	m_Indices = new unsigned int[m_IndexCount];

	unsigned int vertexPointer = 0;
	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
			float yPos = cos(ySegment * PI);
			float zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);

			// positions
			m_Vertices[vertexPointer + 0] = xPos;
			m_Vertices[vertexPointer + 1] = yPos;
			m_Vertices[vertexPointer + 2] = zPos;

			printf("Vertex index=%d, X=%.2ff Y=%.2ff Z=%.2ff\n", vertexPointer, xPos, yPos, zPos);

			// tex coords
			m_Vertices[vertexPointer + 3] = xSegment;
			m_Vertices[vertexPointer + 4] = ySegment;

			// normals
			m_Vertices[vertexPointer + 5] = xPos;
			m_Vertices[vertexPointer + 6] = yPos;
			m_Vertices[vertexPointer + 7] = zPos;

			// tangents
			m_Vertices[vertexPointer + 8] = 0.0f;
			m_Vertices[vertexPointer + 9] = 0.0f;
			m_Vertices[vertexPointer + 10] = 0.0f;

			// bi-tangents
			m_Vertices[vertexPointer + 11] = 0.0f;
			m_Vertices[vertexPointer + 12] = 0.0f;
			m_Vertices[vertexPointer + 13] = 0.0f;

			vertexPointer += vertexStride;
		}
	}

	bool oddRow = false;
	unsigned int indexIndex = 0;
	for (int y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (int x = 0; x <= X_SEGMENTS; ++x)
			{
				m_Indices[indexIndex++] = y       * (X_SEGMENTS + 1) + x;
				printf("Index index=%d, value=%d\n", indexIndex, y * (X_SEGMENTS + 1) + x);
				m_Indices[indexIndex++] = (y + 1) * (X_SEGMENTS + 1) + x;
				printf("Index index=%d, value=%d\n", indexIndex, (y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				m_Indices[indexIndex++] = (y + 1) * (X_SEGMENTS + 1) + x;
				printf("Index index=%d, value=%d\n", indexIndex, (y + 1) * (X_SEGMENTS + 1) + x);
				m_Indices[indexIndex++] = y       * (X_SEGMENTS + 1) + x;
				printf("Index index=%d, value=%d\n", indexIndex, y * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}
}

void Sphere::CreateMesh()
{
	GenerateGeometry();

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_Indices[0]) * m_IndexCount, m_Indices, GL_STATIC_DRAW);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_Vertices[0]) * m_VertexCount, m_Vertices, GL_STATIC_DRAW);

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

void Sphere::RenderMesh()
{
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

void Sphere::ClearMesh()
{
	delete m_Vertices;
	delete m_Indices;

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

Sphere::~Sphere()
{
	ClearMesh();
}
