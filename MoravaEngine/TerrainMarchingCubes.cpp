#include "TerrainMarchingCubes.h"

#include "VertexTBN.h"

#include <limits>


TerrainMarchingCubes::TerrainMarchingCubes() : TerrainVoxel()
{
}

TerrainMarchingCubes::TerrainMarchingCubes(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail) : TerrainVoxel()
{
	m_HeightMapMultiplier = heightMapMultiplier;
	m_IsRequiredMapRebuild = isRequiredMapRebuild;

	m_SeaLevel = seaLevel;

	m_MapGenerator = new MapGenerator(mapGenConf.heightMapFilePath, mapGenConf.colorMapFilePath);
	m_MapGenerator->Generate(mapGenConf, (float)m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);

	m_Scale.x = (float)mapGenConf.mapChunkSize;
	m_Scale.y = (float)m_HeightMapMultiplier;
	m_Scale.z = (float)mapGenConf.mapChunkSize;

	Generate();
}

TerrainMarchingCubes::~TerrainMarchingCubes()
{
	Release();
	delete m_MapGenerator;
}

void TerrainMarchingCubes::Generate(glm::vec3 scale)
{
	Release();

	for (int x = 0; x < (int)m_Scale.x; x++) {
		for (int y = -(int)(m_HeightMapMultiplier / 2); y < (int)(m_HeightMapMultiplier / 2 + 1); y++) {
			for (int z = 0; z < (int)m_Scale.z; z++) {

				float isoSurfaceHeight = y * (1.0f / m_Scale.y);
				glm::vec4 isoSurfaceColor = m_MapGenerator->GetRegionColor(isoSurfaceHeight + 0.5f);
				float heightFinal = m_MapGenerator->m_NoiseMap[x][z];

				if (heightFinal <= m_SeaLevel) {
					heightFinal = m_SeaLevel;
				}

				if (heightFinal >= isoSurfaceHeight + 0.5f) {
					Voxel* voxel = new Voxel();
					int voxelPositionX = x - (int)m_Scale.x / 2;
					if ((int)m_Scale.x % 2 != 0) voxelPositionX += 1;
					int voxelPositionZ = z - (int)m_Scale.z / 2;
					if ((int)m_Scale.z % 2 != 0) voxelPositionZ += 1;

					int voxelPositionY = (int)(isoSurfaceHeight * m_HeightMapMultiplier);
					//	printf("TerrainMarchingCubes::Generate voxelPositionY = %i isoSurfaceHeight = %.2ff m_HeightMapMultiplier = %i\n",
					//		voxelPositionY, isoSurfaceHeight, m_HeightMapMultiplier);

					voxel->position = glm::ivec3(voxelPositionX, voxelPositionY, voxelPositionZ);
					// voxel.color = m_MapGenerator->m_ColorMap[z * m_MapGenerator->m_MapGenConf.mapChunkSize + x];
					voxel->color = isoSurfaceColor;
					voxel->textureID = -1; // no texture
					m_Voxels.push_back(voxel);

					CalculateRanges(x, y, z, isoSurfaceHeight, heightFinal, voxelPositionX, voxelPositionY, voxelPositionZ);

					//	printf("TerrainMarchingCubes::Generate y = %i  heightFinal = %.2ff  m_SeaLevel = %.2ff  isoSurfaceHeight = %.2ff  isoSurfaceColor [ %.2ff %.2ff %.2ff ]\n",
					//		y, heightFinal, m_SeaLevel, isoSurfaceHeight, isoSurfaceColor.r, isoSurfaceColor.g, isoSurfaceColor.b);
					//	printf("m_MapGenerator->m_NoiseMap[%i][%i] = %.2ff\n", x, z, m_MapGenerator->m_NoiseMap[x][z]);
					//	printf("voxel->position = [ %.2ff %.2ff %.2ff ]\n", voxel->position.x, voxel->position.y, voxel->position.z);
				}
			}
		}
	}

	for (auto range : m_Ranges) {
		printf("Variable: %s\tMin: %.2ff\tMax: %.2ff\n", range.first.c_str(), range.second.min, range.second.max);
	}

	MarchingCubes();
}

void TerrainMarchingCubes::MarchingCubes()
{
	int cubeSize = 2; // 2 x 2 x 2 voxels

	auto intMin = std::numeric_limits<int>::min();
	auto intMax = std::numeric_limits<int>::max();

	m_VoxelRangeMin = glm::ivec3(intMax);
	m_VoxelRangeMax = glm::ivec3(intMin);

	// calculate minimum and maximum ranges for all voxels
	for (auto voxel : m_Voxels)
	{
		if (voxel->position.x > m_VoxelRangeMax.x) m_VoxelRangeMax.x = voxel->position.x;
		if (voxel->position.x < m_VoxelRangeMin.x) m_VoxelRangeMin.x = voxel->position.x;

		if (voxel->position.y > m_VoxelRangeMax.y) m_VoxelRangeMax.y = voxel->position.y;
		if (voxel->position.y < m_VoxelRangeMin.y) m_VoxelRangeMin.y = voxel->position.y;

		if (voxel->position.z > m_VoxelRangeMax.z) m_VoxelRangeMax.z = voxel->position.z;
		if (voxel->position.z < m_VoxelRangeMin.z) m_VoxelRangeMin.z = voxel->position.z;
	}

	// printf("TerrainMarchingCubes::MarchingCubes voxelRangeMin [ %.2ff %.2ff %.2ff ] voxelRangeMax [ %.2ff %.2ff %.2ff ]\n",
	// 	m_VoxelRangeMin.x, m_VoxelRangeMin.y, m_VoxelRangeMin.z, m_VoxelRangeMax.x, m_VoxelRangeMax.y, m_VoxelRangeMax.z);

	for (auto vertexPosition : m_VertexPositions)
		delete vertexPosition;
	m_VertexPositions.clear();
	m_EdgePositions.clear();

	m_Triangles.clear();

	// calculate cube parameters for all XYZ positions
	for (int x = (int)m_VoxelRangeMin.x - 1; x < (int)m_VoxelRangeMax.x + 1; x+= cubeSize) {
		for (int y = (int)m_VoxelRangeMin.y - 1; y < (int)m_VoxelRangeMax.y + 1; y += cubeSize) {
			for (int z = (int)m_VoxelRangeMin.z - 1; z < (int)m_VoxelRangeMax.z + 1; z += cubeSize) {

				ComputeSingleCube(x, y, z, cubeSize);
			}
		}
	}

	unsigned int vertexStride = (unsigned int)(sizeof(VertexTBN) / sizeof(float));

	m_IndexCount = (unsigned int)m_Triangles.size() * 3;
	m_VertexCount = m_IndexCount * vertexStride;
	
	m_Vertices = new float[m_VertexCount];
	m_Indices = new unsigned int[m_IndexCount];

	printf("Generate terrain vertices and indices...\n");
	printf("Number of vertices: %d Number of indices: %d\n", m_VertexCount, m_IndexCount);

	// position   tex coords   normal       tangent      bitangent
	// X  Y  Z    U  V         NX  NY  NZ   TX  TY  TZ   BX  BY  BZ
	int vertexPointer = 0;
	int indexPointer = 0;

	for (auto triangle : m_Triangles) {
		for (auto point : triangle.point) {

			// vertex
			m_Vertices[vertexPointer + 0] = (float)point.x;
			m_Vertices[vertexPointer + 1] = (float)point.y;
			m_Vertices[vertexPointer + 2] = (float)point.z;

			// texture coords
			m_Vertices[vertexPointer + 3] = 0.0f;
			m_Vertices[vertexPointer + 4] = 1.0f;

			// normals
			m_Vertices[vertexPointer + 5] = 0.0f;
			m_Vertices[vertexPointer + 6] = 0.0f;
			m_Vertices[vertexPointer + 7] = 0.0f;

			// tangents
			m_Vertices[vertexPointer + 8] = 0.0f;
			m_Vertices[vertexPointer + 9] = 0.0f;
			m_Vertices[vertexPointer + 10] = 0.0f;

			// bitangents
			m_Vertices[vertexPointer + 11] = 0.0f;
			m_Vertices[vertexPointer + 12] = 0.0f;
			m_Vertices[vertexPointer + 13] = 0.0f;

			vertexPointer += vertexStride;

			// Generate terrain indices
			m_Indices[indexPointer] = indexPointer;
			indexPointer++;
		}
	}

	printf("Final value of indexPointer: %d\n", indexPointer);

	unsigned int vertexIndex = 0;
	for (unsigned int i = 0; i < m_VertexCount; i+= vertexStride) {
		//	printf("Vertex [%u] [ %.2ff %.2ff %.2ff ] [ %.2ff %.2ff ] [ %.2ff %.2ff %.2ff ] [ %.2ff %.2ff %.2ff ] [ %.2ff %.2ff %.2ff ]\n", vertexIndex,
		//		m_Vertices[i + 0], m_Vertices[i + 1], m_Vertices[i + 2],
		//		m_Vertices[i + 3], m_Vertices[i + 4],
		//		m_Vertices[i + 5], m_Vertices[i + 6], m_Vertices[i + 7],
		//		m_Vertices[i + 8], m_Vertices[i + 9], m_Vertices[i + 10],
		//		m_Vertices[i + 11], m_Vertices[i + 12], m_Vertices[i + 13]);
		vertexIndex++;
	}

	//	printf("Indices:\n");
	unsigned int indexIndex = 0;
	for (unsigned int i = 0; i < m_IndexCount; i += 3) {
		//	printf("%i, %i, %i,\n", m_Indices[i + 0], m_Indices[i + 1], m_Indices[i + 2]);
	}

	RecalculateNormals();
	RecalculateTangentSpace();

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

void TerrainMarchingCubes::ComputeSingleCube(int x, int y, int z, int cubeSize)
{
	// TODO - calculate parameters for the current marching cube
	m_CubeVertices.clear();
	m_CubeVertices.push_back(glm::ivec3(x, y, z + cubeSize)); // 0
	m_CubeVertices.push_back(glm::ivec3(x + cubeSize, y, z + cubeSize)); // 1
	m_CubeVertices.push_back(glm::ivec3(x + cubeSize, y, z)); // 2
	m_CubeVertices.push_back(glm::ivec3(x, y, z)); // 3
	m_CubeVertices.push_back(glm::ivec3(x, y + cubeSize, z + cubeSize)); // 4
	m_CubeVertices.push_back(glm::ivec3(x + cubeSize, y + cubeSize, z + cubeSize)); // 5
	m_CubeVertices.push_back(glm::ivec3(x + cubeSize, y + cubeSize, z)); // 6
	m_CubeVertices.push_back(glm::ivec3(x, y + cubeSize, z)); // 7

	m_CubeEdges.clear();
	m_CubeEdges.push_back(glm::ivec3(x + cubeSize / 2, y, z + cubeSize)); //  0
	m_CubeEdges.push_back(glm::ivec3(x + cubeSize, y, z + cubeSize / 2)); //  1
	m_CubeEdges.push_back(glm::ivec3(x + cubeSize / 2, y, z)); //  2
	m_CubeEdges.push_back(glm::ivec3(x, y, z + cubeSize / 2)); //  3
	m_CubeEdges.push_back(glm::ivec3(x + cubeSize / 2, y + cubeSize, z + cubeSize)); //  4
	m_CubeEdges.push_back(glm::ivec3(x + cubeSize, y + cubeSize, z + cubeSize / 2)); //  5
	m_CubeEdges.push_back(glm::ivec3(x + cubeSize / 2, y + cubeSize, z)); //  6
	m_CubeEdges.push_back(glm::ivec3(x, y + cubeSize, z + cubeSize / 2)); //  7
	m_CubeEdges.push_back(glm::ivec3(x, y + cubeSize / 2, z + cubeSize)); //  8
	m_CubeEdges.push_back(glm::ivec3(x + cubeSize, y + cubeSize / 2, z + cubeSize)); //  9
	m_CubeEdges.push_back(glm::ivec3(x + cubeSize, y + cubeSize / 2, z)); // 10
	m_CubeEdges.push_back(glm::ivec3(x, y + cubeSize / 2, z)); // 11

	//	for (unsigned int i = 0; i < 8; i++)
	//		printf("Vertex %i [ %.2ff %.2ff %.2ff ] IsVertexAvailable? %s\n",
	//			i, m_CubeVertices[i].x, m_CubeVertices[i].y, m_CubeVertices[i].z, IsVertexAvailable(m_CubeVertices[i]) ? "YES" : "NO");

	for (unsigned int i = 0; i < 8; i++)
		m_VertexPositions.push_back(new VertexMC{ m_CubeVertices[i], IsVertexAvailable(m_CubeVertices[i]) });

	for (unsigned int i = 0; i < 12; i++)
		m_EdgePositions.push_back(m_CubeEdges[i]);

	int edgeTable[256] = {
		0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
		0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
		0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
		0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
		0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
		0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
		0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
		0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
		0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
		0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
		0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
		0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
		0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
		0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
		0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
		0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
		0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
		0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
		0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
		0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
		0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
		0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
		0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
		0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
		0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
		0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
		0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
		0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
		0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
		0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
		0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
		0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
	};

	int triangleTable[256][16] = {
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1 },
		{ 8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1 },
		{ 3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1 },
		{ 4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1 },
		{ 4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1 },
		{ 9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1 },
		{ 10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1 },
		{ 5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1 },
		{ 5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1 },
		{ 8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1 },
		{ 2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1 },
		{ 2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1 },
		{ 11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1 },
		{ 5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1 },
		{ 11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1 },
		{ 11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1 },
		{ 2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1 },
		{ 6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1 },
		{ 3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1 },
		{ 6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1 },
		{ 6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1 },
		{ 8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1 },
		{ 7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1 },
		{ 3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1 },
		{ 0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1 },
		{ 9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1 },
		{ 8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1 },
		{ 5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1 },
		{ 0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1 },
		{ 6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1 },
		{ 10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1 },
		{ 1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1 },
		{ 0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1 },
		{ 3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1 },
		{ 6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1 },
		{ 9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1 },
		{ 8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1 },
		{ 3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1 },
		{ 10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1 },
		{ 10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1 },
		{ 2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1 },
		{ 7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1 },
		{ 2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1 },
		{ 1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1 },
		{ 11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1 },
		{ 8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1 },
		{ 0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1 },
		{ 7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1 },
		{ 7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1 },
		{ 10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1 },
		{ 0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1 },
		{ 7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1 },
		{ 6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1 },
		{ 4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1 },
		{ 10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1 },
		{ 8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1 },
		{ 1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1 },
		{ 10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1 },
		{ 10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1 },
		{ 9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1 },
		{ 7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1 },
		{ 3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1 },
		{ 7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1 },
		{ 3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1 },
		{ 6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1 },
		{ 9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1 },
		{ 1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1 },
		{ 4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1 },
		{ 7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1 },
		{ 6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1 },
		{ 0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1 },
		{ 6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1 },
		{ 0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1 },
		{ 11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1 },
		{ 6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1 },
		{ 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1 },
		{ 9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1 },
		{ 1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1 },
		{ 10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1 },
		{ 0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1 },
		{ 11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1 },
		{ 9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1 },
		{ 7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1 },
		{ 2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1 },
		{ 9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1 },
		{ 9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1 },
		{ 1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1 },
		{ 0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1 },
		{ 10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1 },
		{ 2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1 },
		{ 0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1 },
		{ 0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1 },
		{ 9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1 },
		{ 5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1 },
		{ 5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1 },
		{ 8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1 },
		{ 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1 },
		{ 1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1 },
		{ 3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1 },
		{ 4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1 },
		{ 9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1 },
		{ 11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1 },
		{ 2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1 },
		{ 9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1 },
		{ 3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1 },
		{ 1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1 },
		{ 4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1 },
		{ 0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1 },
		{ 1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
	};

	int cubeIndex = 0;
	if (IsVertexAvailable(m_CubeVertices[0])) cubeIndex |= 1;
	if (IsVertexAvailable(m_CubeVertices[1])) cubeIndex |= 2;
	if (IsVertexAvailable(m_CubeVertices[2])) cubeIndex |= 4;
	if (IsVertexAvailable(m_CubeVertices[3])) cubeIndex |= 8;
	if (IsVertexAvailable(m_CubeVertices[4])) cubeIndex |= 16;
	if (IsVertexAvailable(m_CubeVertices[5])) cubeIndex |= 32;
	if (IsVertexAvailable(m_CubeVertices[6])) cubeIndex |= 64;
	if (IsVertexAvailable(m_CubeVertices[7])) cubeIndex |= 128;

	m_VertexList.clear();
	m_VertexList.resize(12);

	// Cube is entirely in/out of the surface
	if (edgeTable[cubeIndex] == 0) return;

	// Find the vertices where the surface intersects the cube
	if (edgeTable[cubeIndex] &    1) m_VertexList[0] = m_EdgePositions[0];
	if (edgeTable[cubeIndex] &    2) m_VertexList[1] = m_EdgePositions[1];
	if (edgeTable[cubeIndex] &    4) m_VertexList[2] = m_EdgePositions[2];
	if (edgeTable[cubeIndex] &    8) m_VertexList[3] = m_EdgePositions[3];
	if (edgeTable[cubeIndex] &   16) m_VertexList[4] = m_EdgePositions[4];
	if (edgeTable[cubeIndex] &   32) m_VertexList[5] = m_EdgePositions[5];
	if (edgeTable[cubeIndex] &   64) m_VertexList[6] = m_EdgePositions[6];
	if (edgeTable[cubeIndex] &  128) m_VertexList[7] = m_EdgePositions[7];
	if (edgeTable[cubeIndex] &  256) m_VertexList[8] = m_EdgePositions[8];
	if (edgeTable[cubeIndex] &  512) m_VertexList[9] = m_EdgePositions[9];
	if (edgeTable[cubeIndex] & 1024) m_VertexList[10] = m_EdgePositions[10];
	if (edgeTable[cubeIndex] & 2048) m_VertexList[11] = m_EdgePositions[11];

	// Create the triangle
	int nTriang = 0;
	for (int i = 0; triangleTable[cubeIndex][i] != -1; i += 3) {
		Triangle triangle;

		glm::ivec3 vertex_0 = m_VertexList[triangleTable[cubeIndex][i + 0]];
		glm::ivec3 vertex_1 = m_VertexList[triangleTable[cubeIndex][i + 1]];
		glm::ivec3 vertex_2 = m_VertexList[triangleTable[cubeIndex][i + 2]];

		//	printf("ComputeSingleCube vertex_0 [ %i %i %i ]\n", vertex_0.x, vertex_0.y, vertex_0.z);
		//	printf("ComputeSingleCube vertex_1 [ %i %i %i ]\n", vertex_1.x, vertex_1.y, vertex_1.z);
		//	printf("ComputeSingleCube vertex_2 [ %i %i %i ]\n", vertex_2.x, vertex_2.y, vertex_2.z);

		triangle.point[0] = vertex_0;
		triangle.point[1] = vertex_1;
		triangle.point[2] = vertex_2;
		m_Triangles.push_back(triangle);
	}

	//	for (auto triangle : m_Triangles) {
	//		printf("ComputeSingleCube Triangle [ %i %i %i ] [ %i %i %i ] [ %i %i %i ]\n",
	//			triangle.point[0].x, triangle.point[0].y, triangle.point[0].z,
	//			triangle.point[1].x, triangle.point[1].y, triangle.point[1].z,
	//			triangle.point[2].x, triangle.point[2].y, triangle.point[2].z);
	//	};
}

int TerrainMarchingCubes::CalculateCubeIndex()
{
	struct Cube {
		int* values;
	};
	Cube cube;
	cube.values = new int[0];
	int surfaceLevel = 0;

	int cubeIndex = 0;
	for (int i = 0; i < 8; i++) {
		if (cube.values[i] < surfaceLevel) {
			cubeIndex = 1 << i;
		}
	}
	return cubeIndex;
}

bool TerrainMarchingCubes::IsVertexAvailable(glm::ivec3 position)
{
	for (auto voxel : m_Voxels) {
		// printf("TerrainMarchingCubes::IsVertexAvailable voxel->position [ %i %i %i ]\n", voxel->position.x, voxel->position.y, voxel->position.z);
		if (voxel->position == position) {
			// printf("TerrainMarchingCubes::IsVertexAvailable TRUE [ %i %i %i ]\n", position.x, position.y, position.z);
			return true;
		}
	}
	// printf("TerrainMarchingCubes::IsVertexAvailable FALSE [ %i %i %i ]\n", position.x, position.y, position.z);
	return false;
}

void TerrainMarchingCubes::CalculateRanges(int x, int y, int z, float isoSurfaceHeight, float heightFinal, int voxelPositionX, int voxelPositionY, int voxelPositionZ)
{
	auto floatMin = std::numeric_limits<float>::min();
	auto floatMax = std::numeric_limits<float>::max();

	if (m_Ranges.size() == 0) {
		m_Ranges.insert(std::make_pair("x", Range{ floatMax, floatMin }));
		m_Ranges.insert(std::make_pair("y", Range{ floatMax, floatMin }));
		m_Ranges.insert(std::make_pair("z", Range{ floatMax, floatMin }));
		m_Ranges.insert(std::make_pair("isoSurfaceHeight", Range{ floatMax, floatMin }));
		m_Ranges.insert(std::make_pair("heightFinal", Range{ floatMax, floatMin }));
		m_Ranges.insert(std::make_pair("voxelPositionX", Range{ floatMax, floatMin }));
		m_Ranges.insert(std::make_pair("voxelPositionY", Range{ floatMax, floatMin }));
		m_Ranges.insert(std::make_pair("voxelPositionZ", Range{ floatMax, floatMin }));
	}

	if (x > m_Ranges["x"].max) m_Ranges["x"].max = (float)x;
	if (x < m_Ranges["x"].min) m_Ranges["x"].min = (float)x;

	if (y > m_Ranges["y"].max) m_Ranges["y"].max = (float)y;
	if (y < m_Ranges["y"].min) m_Ranges["y"].min = (float)y;

	if (z > m_Ranges["z"].max) m_Ranges["z"].max = (float)z;
	if (z < m_Ranges["z"].min) m_Ranges["z"].min = (float)z;

	if (isoSurfaceHeight > m_Ranges["isoSurfaceHeight"].max) m_Ranges["isoSurfaceHeight"].max = isoSurfaceHeight;
	if (isoSurfaceHeight < m_Ranges["isoSurfaceHeight"].min) m_Ranges["isoSurfaceHeight"].min = isoSurfaceHeight;

	if (heightFinal > m_Ranges["heightFinal"].max) m_Ranges["heightFinal"].max = heightFinal;
	if (heightFinal < m_Ranges["heightFinal"].min) m_Ranges["heightFinal"].min = heightFinal;

	if ((float)voxelPositionX > m_Ranges["voxelPositionX"].max) m_Ranges["voxelPositionX"].max = (float)voxelPositionX;
	if ((float)voxelPositionX < m_Ranges["voxelPositionX"].min) m_Ranges["voxelPositionX"].min = (float)voxelPositionX;

	if ((float)voxelPositionY > m_Ranges["voxelPositionY"].max) m_Ranges["voxelPositionY"].max = (float)voxelPositionY;
	if ((float)voxelPositionY < m_Ranges["voxelPositionY"].min) m_Ranges["voxelPositionY"].min = (float)voxelPositionY;

	if ((float)voxelPositionZ > m_Ranges["voxelPositionZ"].max) m_Ranges["voxelPositionZ"].max = (float)voxelPositionZ;
	if ((float)voxelPositionZ < m_Ranges["voxelPositionZ"].min) m_Ranges["voxelPositionZ"].min = (float)voxelPositionZ;
}

void TerrainMarchingCubes::Update(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail)
{
	m_HeightMapMultiplier = heightMapMultiplier;
	m_IsRequiredMapRebuild = isRequiredMapRebuild;

	m_SeaLevel = seaLevel;

	m_MapGenerator->Generate(mapGenConf, (float)m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);

	m_Scale.x = (float)mapGenConf.mapChunkSize;
	m_Scale.y = (float)m_HeightMapMultiplier;
	m_Scale.z = (float)mapGenConf.mapChunkSize;

	Generate();
}
