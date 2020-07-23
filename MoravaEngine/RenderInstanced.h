#pragma once

#include "TerrainVoxel.h"
#include "Texture.h"
#include "Mesh.h"

#include <map>
#include <vector>


class RenderInstanced
{
public:
	RenderInstanced();
	RenderInstanced(TerrainBase* terrain, Texture* texture, Mesh* mesh);
	void CreateVertexData();
	void Update();
	void Release();
	void SetMouseCursorIntersectPosition(glm::vec3* intersectPosition);
	~RenderInstanced();

	void Render();

private:
	void CreateDataStructure();
	void CreateVertexArray();

public:
	TerrainVoxel* m_Terrain;
	Texture* m_Texture;
	Mesh* m_Mesh;
	unsigned int m_InstanceCount;

private:
	struct Matrix
	{
		glm::vec4 row_0;
		glm::vec4 row_1;
		glm::vec4 row_2;
		glm::vec4 row_3;
	};

	struct InstanceData
	{
		glm::mat4 model; // 16F
		glm::vec4 color; // 4F
	};

	// std::map<Texture*, std::vector<Mesh*>> m_Map; // TODO: Material->Texture
	unsigned int m_VBO_Instanced;
	float* m_VBO_Data;
	unsigned int m_InstanceDataLength; // 16F model view + 4F texOffsets + 1F blendFactor = 21F
	InstanceData* m_InstanceDataArray;
	unsigned int m_BufferSize;

	glm::mat4 m_ModelMatrix;
	glm::vec4 m_InstanceColor;

	glm::vec3* m_MouseCursorIntersectPosition;

};
