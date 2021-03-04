#pragma once

#include "Mesh/Mesh.h"

#include <vector>


class MeshUnity : public Mesh
{
public:
	MeshUnity();
	~MeshUnity();

	void GenerateVertexData(glm::vec3 scale);
	virtual void Generate(glm::vec3 scale) override;
	// virtual void RecalculateNormals() override;

public:
	std::vector<glm::vec3>* vertices;
	std::vector<int>* triangles;
	std::vector<glm::vec2>* uv;
	std::vector<glm::vec3>* normals;

};
