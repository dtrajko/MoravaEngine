#include "MeshGenerator.h"


MeshGenerator::MeshGenerator()
{
}

MeshGenerator::~MeshGenerator()
{
}

MeshData* MeshGenerator::GenerateTerrainMesh(float** heightMap, unsigned int width, unsigned int height, float heightMapMultiplier, float seaLevel, int levelOfDetail)
{
	float topLeftX = (width - 1) / -2.0f;
	float topLeftZ = (height - 1) / 2.0f;

	int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;
	int verticesPerLine = (width - 1) / meshSimplificationIncrement + 1;

	MeshData* meshData = new MeshData(verticesPerLine, verticesPerLine);
	unsigned int vertexIndex = 0;

	// printf("MeshGenerator::GenerateTerrainMesh heightMapMultiplier = %.2ff\n", heightMapMultiplier);

	for (unsigned int y = 0; y < height; y += meshSimplificationIncrement) {
		for (unsigned int x = 0; x < width; x += meshSimplificationIncrement) {

			float heightFinal = heightMap[x][y] - 0.5f;
			heightFinal *= heightMapMultiplier;

			if (heightFinal <= seaLevel) {
				heightFinal = seaLevel;
			}

			glm::vec3 vertex = glm::vec3(topLeftX + x, heightFinal, topLeftZ - y);
			glm::vec2 uv = glm::vec2(x / (float)width, y / (float)height);
			meshData->m_Vertices.push_back(vertex);
			meshData->m_UVs.push_back(uv);
			meshData->m_Normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));

			// for debug purposes
			meshData->m_XY.push_back(glm::vec2(x, y));

			if (x < width - 1 && y < height - 1) {
				meshData->AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
				meshData->AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
			}

			vertexIndex++;
		}
	}

	//	debug
	//	for (unsigned int i = 0; i < meshData->m_Vertices.size(); i++) {
	//		auto vertex = meshData->m_Vertices[i];
	//		auto uv = meshData->m_UVs[i];
	//		auto xy = meshData->m_XY[i];
	//		printf("MeshGenerator [ X=%i Y=%i ] vertex [ %.2ff %.2ff %.2ff ] uv [ %.2ff %.2ff ]\n", (int)xy.x, (int)xy.y, vertex.x, vertex.y, vertex.z, uv.x, uv.y);
	//	}
	//	
	//	for (unsigned int i = 0; i < meshData->m_Triangles.size(); i += 3) {
	//		auto triangle_0 = meshData->m_Triangles[i + 0];
	//		auto triangle_1 = meshData->m_Triangles[i + 1];
	//		auto triangle_2 = meshData->m_Triangles[i + 2];
	//	
	//		printf("MeshData::AddTriangle [ %i %i %i ]\n", triangle_0, triangle_1, triangle_2);
	//	}

	return meshData;
}

MeshData::MeshData(unsigned int meshWidth, unsigned int meshHeight)
{
	m_VerticeIndex = meshWidth * meshHeight;
	m_Vertices = std::vector<glm::vec3>();
	// m_Vertices->resize(m_VerticeIndex);

	m_UVs = std::vector<glm::vec2>();
	// m_UVs->resize(m_VerticeIndex);

	m_TriangleIndex = (meshWidth - 1) * (meshHeight - 1) * 6;
	m_Triangles = std::vector<int>();
	// m_Triangles->resize(m_TriangleIndex);
}

MeshData::~MeshData()
{
}

void MeshData::AddTriangle(int a, int b, int c)
{
	m_Triangles.push_back(a);
	m_Triangles.push_back(b);
	m_Triangles.push_back(c);
	m_TriangleIndex += 3;
}

MeshUnity* MeshData::CreateMesh()
{
	MeshUnity* mesh = new MeshUnity();
	mesh->vertices = &m_Vertices;
	mesh->triangles = &m_Triangles;
	mesh->uv = &m_UVs;
	mesh->normals = &m_Normals;
	mesh->RecalculateNormals();
	mesh->Generate(glm::vec3(1.0f));
	return mesh;
}
