#include "Terrain/MeshGenerator.h"


MeshGenerator::MeshGenerator()
{
}

MeshGenerator::~MeshGenerator()
{
}

MeshUnityData* MeshGenerator::GenerateTerrainMesh(float** heightMap, unsigned int width, unsigned int height, float heightMapMultiplier, float seaLevel, int levelOfDetail)
{
	float topLeftX = (width - 1) / -2.0f;
	float topLeftZ = (height - 1) / 2.0f;

	int meshSimplificationIncrement = (levelOfDetail == 0) ? 1 : levelOfDetail * 2;
	int verticesPerLine = (width - 1) / meshSimplificationIncrement + 1;

	MeshUnityData* meshData = new MeshUnityData(verticesPerLine, verticesPerLine);
	unsigned int vertexIndex = 0;

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

	return meshData;
}

MeshUnityData::MeshUnityData(unsigned int meshWidth, unsigned int meshHeight)
{
	m_Mesh = nullptr;

	m_VerticeIndex = meshWidth * meshHeight;
	m_Vertices = std::vector<glm::vec3>();

	m_UVs = std::vector<glm::vec2>();

	m_TriangleIndex = (meshWidth - 1) * (meshHeight - 1) * 6;
	m_Triangles = std::vector<int>();
}

MeshUnityData::~MeshUnityData()
{
	Release();
}

void MeshUnityData::AddTriangle(int a, int b, int c)
{
	m_Triangles.push_back(a);
	m_Triangles.push_back(b);
	m_Triangles.push_back(c);
	m_TriangleIndex += 3;
}

MeshUnity* MeshUnityData::CreateMesh()
{
	Release();

	m_Mesh = new MeshUnity();
	m_Mesh->vertices = &m_Vertices;
	m_Mesh->triangles = &m_Triangles;
	m_Mesh->uv = &m_UVs;
	m_Mesh->normals = &m_Normals;
	m_Mesh->GenerateVertexData(glm::vec3(1.0f));
	m_Mesh->RecalculateNormals();
	m_Mesh->RecalculateTangentSpace();
	m_Mesh->Generate(glm::vec3(1.0f));
	return m_Mesh;
}

void MeshUnityData::Release()
{
	if (m_Mesh != nullptr) {
		delete m_Mesh;
		m_Mesh = nullptr;
	}
}
