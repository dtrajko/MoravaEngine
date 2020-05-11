#pragma once

#include "Mesh.h"


class Block : public Mesh
{
public:
	Block();
	Block(glm::vec3 scale);
	virtual void Update(glm::vec3 scale) override;
	virtual void Render() override;
	virtual ~Block() override;

private:
	void Generate(glm::vec3 scale);

};
