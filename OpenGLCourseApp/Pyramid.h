#pragma once


#include "Mesh.h"


class Pyramid : public Mesh
{
public:
	Pyramid();
	Pyramid(glm::vec3 scale);
	virtual void Update(glm::vec3 scale) override;
	virtual void Render() override;
	virtual ~Pyramid() override;

private:
	void Generate(glm::vec3 scale);
};
