#pragma once

#include "Mesh/Mesh.h"


class TerrainBase : public Mesh
{
public:
	TerrainBase();
	virtual ~TerrainBase();

	inline virtual void Generate(glm::vec3 scale = glm::vec3(1.0f)) {};
	inline virtual void Release() {};
	virtual float GetMaxY(int x, int z);

};
