#include "Mesh/Mesh.h"

#include "Mesh/VertexTBN.h"

#include <GL/glew.h>

#include <cstdio>


Mesh::Mesh()
{
	m_VAO = 0;
	m_VBO = 0;
	m_IBO = 0;

	m_Vertices = nullptr;
	m_Indices = nullptr;
	m_VertexCount = 0;
	m_IndexCount = 0;
	m_Scale = glm::vec3(1.0f); // obsolete, needs to be removed
	m_Transform = Transform(glm::vec3(0.0f), glm::vec3(0.0f), m_Scale);
}

Mesh::Mesh(glm::vec3 scale)
{
	m_Scale = scale;
}

void Mesh::Create()
{
	Generate(m_Scale);

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
	m_VertexAttribArrayCount++;
	// tex coord
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), (const void*)offsetof(VertexTBN, TexCoord));
	m_VertexAttribArrayCount++;
	// normal
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), (const void*)offsetof(VertexTBN, Normal));
	m_VertexAttribArrayCount++;
	// tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), (const void*)offsetof(VertexTBN, Tangent));
	m_VertexAttribArrayCount++;
	// bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), (const void*)offsetof(VertexTBN, Bitangent));
	m_VertexAttribArrayCount++;

	glBindBuffer(GL_ARRAY_BUFFER, 0);         // Unbind VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

void Mesh::Create(float* vertices, unsigned int* indices, unsigned int vertexCount, unsigned int indexCount)
{
	m_Vertices = vertices;
	m_Indices = indices;
	m_VertexCount = vertexCount;
	m_IndexCount = indexCount;

	Create();
}

void Mesh::Generate(glm::vec3 scale)
{
}

void Mesh::Update(glm::vec3 scale)
{
	if (scale != m_Scale)
	{
		Generate(scale);
	}
}

void Mesh::OnUpdate(H2M::TimestepH2M ts, bool debug)
{
}

void Mesh::Render()
{
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

void Mesh::Clear()
{
	m_Vertices = nullptr;
	m_Indices = nullptr;

	m_IndexCount = 0;
	m_VertexCount = 0;

	if (m_IBO != 0)
	{
		glDeleteBuffers(1, &m_IBO);
		m_IBO = 0;
		printf("Mesh::Clear IBO destroyed...\n");
	}
	if (m_VBO != 0)
	{
		glDeleteBuffers(1, &m_VBO);
		m_VBO = 0;
		printf("Mesh::Clear VBO destroyed...\n");
	}
	if (m_VAO != 0)
	{
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
		printf("Mesh::Clear VAO destroyed...\n");
	}

	for (uint32_t i = 0; i < m_VertexAttribArrayCount; i++)
	{
		// glDisableVertexAttribArray(i);
	}
}

void Mesh::RecalculateNormals()
{
	unsigned int vLength = sizeof(VertexTBN) / sizeof(float);
	unsigned int normalOffset = offsetof(VertexTBN, Normal) / sizeof(float);

	// The Phong shading approach
	for (size_t i = 0; i < m_IndexCount; i += 3)
	{
		unsigned int in0 = m_Indices[i + 0] * vLength;
		unsigned int in1 = m_Indices[i + 1] * vLength;
		unsigned int in2 = m_Indices[i + 2] * vLength;
		glm::vec3 v1(m_Vertices[in1 + 0] - m_Vertices[in0 + 0], m_Vertices[in1 + 1] - m_Vertices[in0 + 1], m_Vertices[in1 + 2] - m_Vertices[in0 + 2]);
		glm::vec3 v2(m_Vertices[in2 + 0] - m_Vertices[in0 + 0], m_Vertices[in2 + 1] - m_Vertices[in0 + 1], m_Vertices[in2 + 2] - m_Vertices[in0 + 2]);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		in0 += normalOffset;
		in1 += normalOffset;
		in2 += normalOffset;

		m_Vertices[in0 + 0] += normal.x; m_Vertices[in0 + 1] += normal.y; m_Vertices[in0 + 2] += normal.z;
		m_Vertices[in1 + 0] += normal.x; m_Vertices[in1 + 1] += normal.y; m_Vertices[in1 + 2] += normal.z;
		m_Vertices[in2 + 0] += normal.x; m_Vertices[in2 + 1] += normal.y; m_Vertices[in2 + 2] += normal.z;
	}

	for (unsigned int i = 0; i < m_VertexCount / vLength; i++)
	{
		unsigned int nOffset = i * vLength + normalOffset;
		glm::vec3 vec(m_Vertices[nOffset + 0], m_Vertices[nOffset + 1], m_Vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		m_Vertices[nOffset + 0] = vec.x; m_Vertices[nOffset + 1] = vec.y; m_Vertices[nOffset + 2] = vec.z;
	}
}

void Mesh::RecalculateTangentSpace()
{
	unsigned int vLength = sizeof(VertexTBN) / sizeof(float);

	for (size_t i = 0; i < m_IndexCount; i += 3)
	{
		unsigned int in0 = m_Indices[i + 0] * vLength;
		unsigned int in1 = m_Indices[i + 1] * vLength;
		unsigned int in2 = m_Indices[i + 2] * vLength;
		glm::vec3 v0(m_Vertices[in0 + 0], m_Vertices[in0 + 1], m_Vertices[in0 + 2]);
		glm::vec3 v1(m_Vertices[in1 + 0], m_Vertices[in1 + 1], m_Vertices[in1 + 2]);
		glm::vec3 v2(m_Vertices[in0 + 2], m_Vertices[in2 + 1], m_Vertices[in2 + 2]);

		glm::vec2 uv0(m_Vertices[in0 + 3], m_Vertices[in0 + 4]);
		glm::vec2 uv1(m_Vertices[in1 + 3], m_Vertices[in1 + 4]);
		glm::vec2 uv2(m_Vertices[in2 + 3], m_Vertices[in2 + 4]);

		// Edges of the triangle : position delta
		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v1;

		// UV delta
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

		// write tangents
		m_Vertices[in0 + 8] = tangent.x; m_Vertices[in0 + 9] = tangent.y; m_Vertices[in0 + 10] = tangent.z;
		m_Vertices[in1 + 8] = tangent.x; m_Vertices[in1 + 9] = tangent.y; m_Vertices[in1 + 10] = tangent.z;
		m_Vertices[in2 + 8] = tangent.x; m_Vertices[in2 + 9] = tangent.y; m_Vertices[in2 + 10] = tangent.z;

		// write bitangents
		m_Vertices[in0 + 11] = bitangent.x; m_Vertices[in0 + 12] = bitangent.y; m_Vertices[in0 + 13] = bitangent.z;
		m_Vertices[in1 + 11] = bitangent.x; m_Vertices[in1 + 12] = bitangent.y; m_Vertices[in1 + 13] = bitangent.z;
		m_Vertices[in2 + 11] = bitangent.x; m_Vertices[in2 + 12] = bitangent.y; m_Vertices[in2 + 13] = bitangent.z;
	}
}

void Mesh::BindVertexArray()
{
	glBindVertexArray(m_VAO);
}

Mesh::~Mesh()
{
	Clear();
}
