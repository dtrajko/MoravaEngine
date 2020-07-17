#pragma once

#include "Texture.h"
#include "Mesh.h"

#include <map>
#include <vector>


class RenderInstanced
{
public:
	RenderInstanced();
	void CreateVertexAttributes(std::vector<glm::vec3> positions);
	void SetMesh(Texture* texture, Mesh* mesh);
	~RenderInstanced();

	void Render();

public:
	unsigned int m_InstanceCount;
	Texture* m_Texture;
	Mesh* m_Mesh;

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
		Matrix model; // 16F
	};

	// std::map<Texture*, std::vector<Mesh*>> m_Map; // TODO: Material->Texture
	unsigned int m_VBO_Instanced;
	float* m_VBO_Data;
	unsigned int m_InstanceDataLength; // 16F model view + 4F texOffsets + 1F blendFactor = 21F
	glm::mat4 m_ModelMatrix;
	glm::mat4* m_ModelMatrices;

};
