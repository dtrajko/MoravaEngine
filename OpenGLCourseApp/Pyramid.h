#pragma once


#include "Mesh.h"


class Pyramid : public Mesh
{
public:
	Pyramid();
	Pyramid(glm::vec3 scale);
	virtual void Generate(glm::vec3 scale) override;
	virtual ~Pyramid() override;

};
