#pragma once

#include "Mesh.h"


class TerrainBase : public Mesh
{
public:
	TerrainBase();
	virtual ~TerrainBase();

	virtual void Generate();
	virtual float GetMaxY(int x, int z);

};
