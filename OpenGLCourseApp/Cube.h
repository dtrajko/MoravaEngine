#pragma once

#include "Mesh.h"


class Cube : public Mesh
{
public:
	Cube();
	virtual void Render() override;
	virtual ~Cube() override;

};
