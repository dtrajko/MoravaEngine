#include "Mesh.h"

#include <GL/glew.h>

#include "VertexTiling.h"


Mesh::Mesh()
{
	m_VAO = 0;
	m_VBO = 0;
	m_IBO = 0;
	m_IndexCount = 0;
}

void Mesh::Create(float* vertices, unsigned int* indices, unsigned int vertexCount, unsigned int indexCount)
{
	m_IndexCount = indexCount;

	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glGenBuffers(1, &m_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indexCount, indices, GL_STATIC_DRAW);

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

void Mesh::CalcAverageNormals(unsigned int* indices, unsigned int indexCount, float* vertices, unsigned int vertexCount)
{
	unsigned int vLength = sizeof(VertexTiling) / sizeof(float);
	unsigned int normalOffset = offsetof(VertexTiling, Normal) / sizeof(float);

	// The Phong shading approach
	for (size_t i = 0; i < indexCount; i += 3)
	{
		unsigned int in0 = indices[i + 0] * vLength;
		unsigned int in1 = indices[i + 1] * vLength;
		unsigned int in2 = indices[i + 2] * vLength;
		glm::vec3 v1(vertices[in1 + 0] - vertices[in0 + 0], vertices[in1 + 1] - vertices[in0 + 1], vertices[in1 + 2] - vertices[in0 + 2]);
		glm::vec3 v2(vertices[in2 + 0] - vertices[in0 + 0], vertices[in2 + 1] - vertices[in0 + 1], vertices[in2 + 2] - vertices[in0 + 2]);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		in0 += normalOffset;
		in1 += normalOffset;
		in2 += normalOffset;

		vertices[in0 + 0] += normal.x; vertices[in0 + 1] += normal.y; vertices[in0 + 2] += normal.z;
		vertices[in1 + 0] += normal.x; vertices[in1 + 1] += normal.y; vertices[in1 + 2] += normal.z;
		vertices[in2 + 0] += normal.x; vertices[in2 + 1] += normal.y; vertices[in2 + 2] += normal.z;
	}

	for (unsigned int i = 0; i < vertexCount / vLength; i++)
	{
		unsigned int nOffset = i * vLength + normalOffset;
		glm::vec3 vec(vertices[nOffset + 0], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		vertices[nOffset + 0] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
	}
}

void Mesh::CalcTangentSpace(unsigned int* indices, unsigned int indexCount, float* vertices, unsigned int vertexCount)
{
	unsigned int vLength = sizeof(VertexTiling) / sizeof(float);

	for (size_t i = 0; i < indexCount; i += 3)
	{
		unsigned int in0 = indices[i + 0] * vLength;
		unsigned int in1 = indices[i + 1] * vLength;
		unsigned int in2 = indices[i + 2] * vLength;
		glm::vec3 v0(vertices[in0 + 0], vertices[in0 + 1], vertices[in0 + 2]);
		glm::vec3 v1(vertices[in1 + 0], vertices[in1 + 1], vertices[in1 + 2]);
		glm::vec3 v2(vertices[in0 + 2], vertices[in2 + 1], vertices[in2 + 2]);

		glm::vec2 uv0(vertices[in0 + 3], vertices[in0 + 4]);
		glm::vec2 uv1(vertices[in1 + 3], vertices[in1 + 4]);
		glm::vec2 uv2(vertices[in2 + 3], vertices[in2 + 4]);

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
		vertices[in0 + 8] = tangent.x; vertices[in0 + 9] = tangent.y; vertices[in0 + 10] = tangent.z;
		vertices[in1 + 8] = tangent.x; vertices[in1 + 9] = tangent.y; vertices[in1 + 10] = tangent.z;
		vertices[in2 + 8] = tangent.x; vertices[in2 + 9] = tangent.y; vertices[in2 + 10] = tangent.z;

		// write bitangents
		vertices[in0 + 11] = bitangent.x; vertices[in0 + 12] = bitangent.y; vertices[in0 + 13] = bitangent.z;
		vertices[in1 + 11] = bitangent.x; vertices[in1 + 12] = bitangent.y; vertices[in1 + 13] = bitangent.z;
		vertices[in2 + 11] = bitangent.x; vertices[in2 + 12] = bitangent.y; vertices[in2 + 13] = bitangent.z;

		vertices[14] = 1.0f;
	}
}

Mesh::~Mesh()
{
	Clear();
}
