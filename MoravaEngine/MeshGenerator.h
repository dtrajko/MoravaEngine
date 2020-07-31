#pragma once


/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class MeshGenerator
{
public:
	MeshGenerator();
	~MeshGenerator();

	static void GenerateTerrainMesh(float** heightMap, unsigned int width, unsigned int height);

};
