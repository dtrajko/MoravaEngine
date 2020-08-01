#include "MeshUnity.h"

#include "VertexTBN.h"


MeshUnity::MeshUnity()
{
}

MeshUnity::~MeshUnity()
{
}

void MeshUnity::RecalculateNormals()
{
	// The Phong shading approach
	for (size_t i = 0; i < triangles->size(); i += 3)
	{
		glm::vec3 v1(vertices->at(i + 1).x - vertices->at(i + 0).x, vertices->at(i + 1).y - vertices->at(i + 0).y, vertices->at(i + 1).z - vertices->at(i + 0).z);
		glm::vec3 v2(vertices->at(i + 2).x - vertices->at(i + 0).x, vertices->at(i + 2).y - vertices->at(i + 0).y, vertices->at(i + 2).z - vertices->at(i + 0).z);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		normals->at(i + 0).x += normal.x;
		normals->at(i + 0).y += normal.y;
		normals->at(i + 0).z += normal.z;

		normals->at(i + 1).x += normal.x;
		normals->at(i + 1).y += normal.y;
		normals->at(i + 1).z += normal.z;

		normals->at(i + 2).x += normal.x;
		normals->at(i + 2).y += normal.y;
		normals->at(i + 2).z += normal.z;
	}

	for (unsigned int i = 0; i < normals->size(); i++)
	{
		glm::vec3 normal = glm::normalize(normals->at(i));
		normals->at(i).x = normal.x;
		normals->at(i).y = normal.y;
		normals->at(i).z = normal.z;
	}
}
