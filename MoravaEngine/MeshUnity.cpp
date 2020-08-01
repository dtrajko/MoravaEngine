#include "MeshUnity.h"

#include "VertexTBN.h"

#include "GL/glew.h"


MeshUnity::MeshUnity()
{
}

MeshUnity::~MeshUnity()
{
}

void MeshUnity::Generate(glm::vec3 scale)
{
	m_VertexCount = (unsigned int)(vertices->size() * (sizeof(VertexTBN) / sizeof(float)));

	printf("MeshUnity::Generate vertices->size: %zu vertex stripe size: %zu\n", vertices->size(), sizeof(VertexTBN) / sizeof(float));

	m_Vertices = new float[m_VertexCount];

	//  0  1  2    3  4     5   6   7     8   9  10   11  12  13
	// [X, Y, Z]  [U, V]  [NX, NY, NZ]  [TX, TY, TZ]  [BX, BY, BZ]

	for (unsigned int i = 0; i < vertices->size(); i++) {

		// vertex positions
		m_Vertices[i + 0] = vertices->at(i).x;
		m_Vertices[i + 1] = vertices->at(i).y;
		m_Vertices[i + 2] = vertices->at(i).z;

		// texture coords
		m_Vertices[i + 3] = uv->at(i).x;
		m_Vertices[i + 4] = uv->at(i).y;

		// normals
		m_Vertices[i + 5] = normals->at(i).x;
		m_Vertices[i + 6] = normals->at(i).y;
		m_Vertices[i + 7] = normals->at(i).z;

		// tangents
		m_Vertices[i + 8] = 0.0f;
		m_Vertices[i + 9] = 0.0f;
		m_Vertices[i + 10] = 0.0f;

		// bitangents
		m_Vertices[i + 11] = 0.0f;
		m_Vertices[i + 12] = 0.0f;
		m_Vertices[i + 13] = 0.0f;
	}

	m_IndexCount = (unsigned int)triangles->size();

	m_Indices = new unsigned int[m_IndexCount];

	for (unsigned int i = 0; i < triangles->size(); i++) {
		m_Indices[i] = triangles->at(i);
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
}

void MeshUnity::RecalculateNormals()
{
	// The Phong shading approach
	//	for (size_t i = 0; i < triangles->size(); i++)
	//	{
	//		glm::vec3 v1(vertices->at(i + 1).x - vertices->at(i + 0).x, vertices->at(i + 1).y - vertices->at(i + 0).y, vertices->at(i + 1).z - vertices->at(i + 0).z);
	//		glm::vec3 v2(vertices->at(i + 2).x - vertices->at(i + 0).x, vertices->at(i + 2).y - vertices->at(i + 0).y, vertices->at(i + 2).z - vertices->at(i + 0).z);
	//		glm::vec3 normal = glm::cross(v1, v2);
	//		normal = glm::normalize(normal);
	//	
	//		normals->at(i + 0).x += normal.x;
	//		normals->at(i + 0).y += normal.y;
	//		normals->at(i + 0).z += normal.z;
	//	
	//		normals->at(i + 1).x += normal.x;
	//		normals->at(i + 1).y += normal.y;
	//		normals->at(i + 1).z += normal.z;
	//	
	//		normals->at(i + 2).x += normal.x;
	//		normals->at(i + 2).y += normal.y;
	//		normals->at(i + 2).z += normal.z;
	//	}
	//	
	//	for (unsigned int i = 0; i < normals->size(); i++)
	//	{
	//		glm::vec3 normal = glm::normalize(normals->at(i));
	//		normals->at(i).x = normal.x;
	//		normals->at(i).y = normal.y;
	//		normals->at(i).z = normal.z;
	//	}
}
