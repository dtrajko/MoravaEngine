#pragma once

#include "Mesh.h"


class Block : public Mesh
{
public:
	Block();
	Block(glm::vec3 scale);
	virtual void Render() override;
	virtual ~Block() override;
	virtual void Generate(glm::vec3 scale) override;

};
