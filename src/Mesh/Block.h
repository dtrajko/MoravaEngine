#pragma once

#include "Mesh/Mesh.h"


class Block : public Mesh
{
public:
	Block();
	Block(glm::vec3 scale);
	virtual ~Block() override;

	virtual void Render() override;
	virtual void Generate(glm::vec3 scale) override;

};
