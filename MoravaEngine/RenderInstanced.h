#pragma once

#include "Texture.h"
#include "Mesh.h"

#include <map>
#include <vector>


class RenderInstanced
{
public:
	RenderInstanced();
	void CreateVertexAttributesI();
	void CreateVertexAttributesII();
	~RenderInstanced();

	void Render();

private:
	std::map<Texture*, std::vector<Mesh*>*> m_Map; // TODO: Material->Texture
	unsigned int m_VAO; // it actually belongs to mesh
	float* m_VBO_Data;
	unsigned int m_VBO_Instanced;
	int INSTANCE_DATA_LENGTH = 21; // 16F model view + 4F texOffsets + 1F blendFactor
	unsigned int m_InstanceCount = 50000;
	glm::mat4* m_ModelMatrices;

};
