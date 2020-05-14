#include "Sphere.h"

#include <cmath>

#include "VertexTBN.h"


Sphere::Sphere() : Mesh()
{
	m_VAO = 0;
	m_VBO = 0;
	m_IBO = 0;
	m_Vertices = nullptr;
	m_Indices = nullptr;
	m_VertexCount = 0;
	m_IndexCount = 0;
	m_Scale = glm::vec3(1.0f);

	Create();
}

Sphere::Sphere(glm::vec3 scale)
{
	m_Scale = scale;
	Create();
}

void Sphere::Generate(glm::vec3 scale)
{
	float maxScale = scale.x;
	if (scale.y > maxScale) maxScale = scale.y;
	if (scale.z > maxScale) maxScale = scale.z;

	m_Radius = 0.5f * maxScale;

	// printf("Sphere::Generate scale [ %2.ff %2.ff %2.ff ] m_Scale [ %2.ff %2.ff %2.ff ]\n", scale.x, scale.y, scale.z, m_Scale.x, m_Scale.y, m_Scale.z);
	// printf("Sphere::Generate maxScale=%.2f radius=%.2f\n", maxScale, m_Radius);

	m_Scale = scale;

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
	m_VertexCount = sizeof(VertexTBN) * (sectorCount + 1) * (stackCount + 1);
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
}

Sphere::~Sphere()
{
	Clear();
}
