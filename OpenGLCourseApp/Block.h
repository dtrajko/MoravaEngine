#pragma once

#include "Mesh.h"



class Block : public Mesh
{
public:
	Block();
	Block(float scaleX, float scaleY, float scaleZ);
	virtual void Render() override;
	virtual ~Block() override;

};
