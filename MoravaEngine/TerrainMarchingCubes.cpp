#include "TerrainMarchingCubes.h"

#include <limits>


TerrainMarchingCubes::TerrainMarchingCubes() : TerrainVoxel()
{
}

TerrainMarchingCubes::TerrainMarchingCubes(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail) : TerrainVoxel()
{
	m_HeightMapMultiplier = heightMapMultiplier;
	m_IsRequiredMapRebuild = isRequiredMapRebuild;

	m_SeaLevel = seaLevel;

	m_MapGenerator = new MapGenerator(mapGenConf.heightMapFilePath, mapGenConf.colorMapFilePath);
	m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);

	m_Scale.x = (float)mapGenConf.mapChunkSize;
	m_Scale.y = m_HeightMapMultiplier;
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

	for (int x = 0; x < m_Scale.x; x++) {
		for (int y = -(int)(m_HeightMapMultiplier / 2.0f); y < (int)(m_HeightMapMultiplier / 2.0f + 1.0f); y++) {
			for (int z = 0; z < m_Scale.z; z++) {

				float isoSurfaceHeight = y * (1.0f / m_Scale.y);
				glm::vec4 isoSurfaceColor = m_MapGenerator->GetRegionColor(isoSurfaceHeight + 0.5f);
				float heightFinal = m_MapGenerator->m_NoiseMap[x][z];

				if (heightFinal <= m_SeaLevel) {
					heightFinal = m_SeaLevel;
				}

				if (heightFinal >= isoSurfaceHeight + 0.5f) {
					Voxel* voxel = new Voxel();
					float voxelPositionX = x - m_Scale.x / 2.0f;
					if ((int)m_Scale.x % 2 != 0) voxelPositionX += 0.5f;
					float voxelPositionZ = z - m_Scale.z / 2.0f;
					if ((int)m_Scale.z % 2 != 0) voxelPositionZ += 0.5f;

					float voxelPositionY = isoSurfaceHeight * m_HeightMapMultiplier;

					voxel->position = glm::vec3(voxelPositionX, voxelPositionY, voxelPositionZ);
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

	auto floatMin = std::numeric_limits<float>::min();
	auto floatMax = std::numeric_limits<float>::max();

	m_VoxelRangeMin = glm::vec3(floatMax);
	m_VoxelRangeMax = glm::vec3(floatMin);

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

	// calculate cube parameters for all XYZ positions
	for (int x = (int)m_VoxelRangeMin.x - 1; x < (int)m_VoxelRangeMax.x + 1; x+= cubeSize) {
		for (int y = (int)m_VoxelRangeMin.y - 1; y < (int)m_VoxelRangeMax.y + 1; y += cubeSize) {
			for (int z = (int)m_VoxelRangeMin.z - 1; z < (int)m_VoxelRangeMax.z + 1; z += cubeSize) {
				//	printf("TerrainMarchingCubes::MarchingCubes Cube position Min [ %i %i %i ] Max [ %i %i %i ]\n", 
				//		x, y, z, x + cubeSize, y + cubeSize, z + cubeSize);

				// TODO - calculate parameters for the current marching cube
				m_CubeVertices.clear();
				m_CubeVertices.push_back(glm::vec3(x, y, z + cubeSize));
				m_CubeVertices.push_back(glm::vec3(x + cubeSize, y, z + cubeSize));
				m_CubeVertices.push_back(glm::vec3(x + cubeSize, y, z));
				m_CubeVertices.push_back(glm::vec3(x, y, z));
				m_CubeVertices.push_back(glm::vec3(x, y + cubeSize, z + cubeSize));
				m_CubeVertices.push_back(glm::vec3(x + cubeSize, y + cubeSize, z + cubeSize));
				m_CubeVertices.push_back(glm::vec3(x + cubeSize, y + cubeSize, z));
				m_CubeVertices.push_back(glm::vec3(x, y + cubeSize, z));

				//	for (unsigned int i = 0; i < 8; i++)
				//		printf("Vertex %i [ %.2ff %.2ff %.2ff ] IsVertexAvailable? %s\n",
				//			i, m_CubeVertices[i].x, m_CubeVertices[i].y, m_CubeVertices[i].z, IsVertexAvailable(m_CubeVertices[i]) ? "YES" : "NO");

				for (unsigned int i = 0; i < 8; i++)
					m_VertexPositions.push_back(new VertexMC{ m_CubeVertices[i], IsVertexAvailable(m_CubeVertices[i]) });
			}
		}
	}
}

int TerrainMarchingCubes::CalculateCubeIndex()
{
	struct Cube {
		int* values;
	};
	Cube cube;
	int surfaceLevel;

	int cubeIndex = 0;
	for (int i = 0; i < 8; i++) {
		if (cube.values[i] < surfaceLevel) {
			cubeIndex = 1 << i;
		}
	}
	return cubeIndex;
}

bool TerrainMarchingCubes::IsVertexAvailable(glm::vec3 position)
{
	for (auto voxel : m_Voxels) {
		if (voxel->position == position)
			return true;
	}
	return false;
}

void TerrainMarchingCubes::CalculateRanges(int x, int y, int z, float isoSurfaceHeight, float heightFinal, float voxelPositionX, float voxelPositionY, float voxelPositionZ)
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

	if (voxelPositionX > m_Ranges["voxelPositionX"].max) m_Ranges["voxelPositionX"].max = voxelPositionX;
	if (voxelPositionX < m_Ranges["voxelPositionX"].min) m_Ranges["voxelPositionX"].min = voxelPositionX;

	if (voxelPositionY > m_Ranges["voxelPositionY"].max) m_Ranges["voxelPositionY"].max = voxelPositionY;
	if (voxelPositionY < m_Ranges["voxelPositionY"].min) m_Ranges["voxelPositionY"].min = voxelPositionY;

	if (voxelPositionZ > m_Ranges["voxelPositionZ"].max) m_Ranges["voxelPositionZ"].max = voxelPositionZ;
	if (voxelPositionZ < m_Ranges["voxelPositionZ"].min) m_Ranges["voxelPositionZ"].min = voxelPositionZ;
}

void TerrainMarchingCubes::Update(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail)
{
	m_HeightMapMultiplier = heightMapMultiplier;
	m_IsRequiredMapRebuild = isRequiredMapRebuild;

	m_SeaLevel = seaLevel;

	m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);

	m_Scale.x = (float)mapGenConf.mapChunkSize;
	m_Scale.y = m_HeightMapMultiplier;
	m_Scale.z = (float)mapGenConf.mapChunkSize;

	Generate();
}
