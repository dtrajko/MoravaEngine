#pragma once

#include "Mesh.h"


class Quad : public Mesh
{
public:
	Quad();
	virtual void Render() override;
	virtual ~Quad();

};
