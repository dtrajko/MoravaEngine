#pragma once

#include "Mesh.h"


class Quad : public Mesh
{
public:
	Quad();
	virtual ~Quad() override;

	virtual void Render() override;

};
