#include "Ring.h"

#include <GL/glew.h>

#include <vector>

#include "VertexTBN.h"


Ring::Ring() : Mesh()
{
	m_Scale = glm::vec3(1.0f);
}

Ring::Ring(glm::vec3 scale)
{
	m_Scale = scale;
	Generate(scale);
}

void Ring::Generate(glm::vec3 scale)
{
	m_Scale = scale;


	for (int i = 0; i <= m_Slices; ++i)
	{
		const float angle = (static_cast<float>(i % m_Slices) * m_Tau / m_Slices) + (m_Tau / 8.f);
		const float c = std::cos(angle);
		const float s = std::sin(angle);

		//	const float3x2 mat = { axis, arm1 * c + arm2 * s };
		//	for (auto& p : points) mesh.vertices.push_back({ mul(mat, p) + eps, float3(0.f) });
		//	
		//	if (i > 0)
		//	{
		//		for (uint32_t j = 1; j < (uint32_t)points.size(); ++j)
		//		{
		//			uint32_t i0 = (i - 1) * uint32_t(points.size()) + (j - 1);
		//			uint32_t i1 = (i - 0) * uint32_t(points.size()) + (j - 1);
		//			uint32_t i2 = (i - 0) * uint32_t(points.size()) + (j - 0);
		//			uint32_t i3 = (i - 1) * uint32_t(points.size()) + (j - 0);
		//			mesh.triangles.push_back({ i0,i1,i2 });
		//			mesh.triangles.push_back({ i0,i2,i3 });
		//		}
		//	}
	}

	// ComputeNormals(mesh);

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

Ring::~Ring()
{
}

void Ring::ComputeNormals()
{
	//	static const double NORMAL_EPSILON = 0.0001;
	//	
	//	std::vector<uint32_t> uniqueVertIndices(mesh.vertices.size(), 0);
	//	for (uint32_t i = 0; i < uniqueVertIndices.size(); ++i)
	//	{
	//		if (uniqueVertIndices[i] == 0)
	//		{
	//			uniqueVertIndices[i] = i + 1;
	//			const glm::vec3 v0 = mesh.vertices[i].position;
	//			for (auto j = i + 1; j < mesh.vertices.size(); ++j)
	//			{
	//				const glm::vec3 v1 = mesh.vertices[j].position;
	//				if (length2(v1 - v0) < NORMAL_EPSILON)
	//				{
	//					uniqueVertIndices[j] = uniqueVertIndices[i];
	//				}
	//			}
	//		}
	//	}
	//	
	//	uint32_t idx0, idx1, idx2;
	//	for (auto& t : mesh.triangles)
	//	{
	//		idx0 = uniqueVertIndices[t.x] - 1;
	//		idx1 = uniqueVertIndices[t.y] - 1;
	//		idx2 = uniqueVertIndices[t.z] - 1;
	//	
	//		geometry_vertex& v0 = mesh.vertices[idx0], & v1 = mesh.vertices[idx1], & v2 = mesh.vertices[idx2];
	//		const glm::vec3 n = cross(v1.position - v0.position, v2.position - v0.position);
	//		v0.normal += n; v1.normal += n; v2.normal += n;
	//	}
	//	
	//	for (uint32_t i = 0; i < mesh.vertices.size(); ++i) mesh.vertices[i].normal = mesh.vertices[uniqueVertIndices[i] - 1].normal;
	//	for (geometry_vertex& v : mesh.vertices) v.normal = normalize(v.normal);
}
