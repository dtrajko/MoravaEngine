#include "Mesh/Sphere.h"

#include "Mesh/VertexTBN.h"

#include <cmath>


Sphere::Sphere() : Sphere(glm::vec3(1.0f))
{
}

Sphere::Sphere(glm::vec3 scale) : Mesh()
{
	Generate(scale);
}

void Sphere::Generate(glm::vec3 scale)
{
	if (m_VertexCount > 0 || m_IndexCount > 0) return;

	m_Scale = scale;

	float maxScale = scale.x;
	if (scale.y > maxScale) maxScale = scale.y;
	if (scale.z > maxScale) maxScale = scale.z;

	m_Radius = 0.5f * maxScale;
	if (m_Radius == m_RadiusPrev) {
		return;
	}
	m_RadiusPrev = m_Radius;

	// printf("Sphere::Generate m_Radius = %.2ff\n", m_Radius);

	const unsigned int sectorCount = 32;
	const unsigned int stackCount = 32;
	const float PI = 3.14159265359f;

	float x, y, z, xy; // vertex position
	float nx, ny, nz, lengthInv = 1.0f / m_Radius; // vertex normal
	float s, t; // vertex texCoord

	float sectorStep = 2 * PI / sectorCount;
	float stackStep = PI / stackCount;
	float sectorAngle, stackAngle;

	unsigned int vertexStride = (unsigned int)(sizeof(VertexTBN) / sizeof(float));
	unsigned int m_VertexCount = sizeof(VertexTBN) * (sectorCount + 1) * (stackCount + 1);
	m_IndexCount = 6 * sectorCount * (stackCount - 1);

	m_Vertices = new float[m_VertexCount];
	m_Indices = new unsigned int[m_IndexCount];

	unsigned int vertexPointer = 0;
	for (int i = 0; i <= stackCount; ++i)
	{
		stackAngle = PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
		xy = m_Radius * cosf(stackAngle); // r * cos(u)
		z = m_Radius * sinf(stackAngle); // r * sin(u)

		// add (sectorCount+1) vertices per stack
		// the first and last vertices have same position and normal, but different tex coords
		for (int j = 0; j <= sectorCount; ++j)
		{
			sectorAngle = j * sectorStep; // starting from 0 to 2pi

			// positions
			x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
			m_Vertices[vertexPointer + 0] = x;
			m_Vertices[vertexPointer + 1] = y;
			m_Vertices[vertexPointer + 2] = z;

			// printf("Vertex index=%d, X=%.2ff Y=%.2ff Z=%.2ff\n", vertexPointer, x, y, z);

			// vertex tex coord (s, t) range between [0, 1]
			s = (float)j / sectorCount;
			t = (float)i / stackCount;
			m_Vertices[vertexPointer + 3] = s;
			m_Vertices[vertexPointer + 4] = t;

			// normalized vertex normal (nx, ny, nz)
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;
			m_Vertices[vertexPointer + 5] = nx;
			m_Vertices[vertexPointer + 6] = ny;
			m_Vertices[vertexPointer + 7] = nz;

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

	m_VertexCount = vertexPointer;

	// generate CCW index list of sphere triangles
	int k1, k2;
	unsigned int indexIndex = 0;
	for (int i = 0; i < stackCount; ++i)
	{
		k1 = i * (sectorCount + 1); // beginning of current stack
		k2 = k1 + sectorCount + 1;  // beginning of next stack

		for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0)
			{
				m_Indices[indexIndex++] = k1;
				m_Indices[indexIndex++] = k2;
				m_Indices[indexIndex++] = k1 + 1;
			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1))
			{
				m_Indices[indexIndex++] = k1 + 1;
				m_Indices[indexIndex++] = k2;
				m_Indices[indexIndex++] = k2 + 1;
			}
		}
	}

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

	delete[] m_Vertices;
	delete[] m_Indices;
}

Sphere::~Sphere()
{
}
