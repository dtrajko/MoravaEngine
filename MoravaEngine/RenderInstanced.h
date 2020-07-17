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
	~RenderInstanced();

	void Render();

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

	std::map<Texture*, std::vector<Mesh*>*> m_Map; // TODO: Material->Texture
	std::vector<Mesh*> meshVector;
	unsigned int m_VAO; // it actually belongs to mesh
	unsigned int m_VBO_Instanced;
	float* m_VBO_Data;
	unsigned int m_InstanceDataLength; // 16F model view + 4F texOffsets + 1F blendFactor = 21F
	unsigned int m_InstanceCount = 50000;
	glm::mat4 m_ModelMatrix;
	glm::mat4* m_ModelMatrices;

};
