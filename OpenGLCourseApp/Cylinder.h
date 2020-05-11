#pragma once

#include <glm/glm.hpp>

#include "Mesh.h"


class Cylinder : public Mesh
{
public:
	Cylinder();
	Cylinder(glm::vec3 scale);
	void Update(glm::vec3 scale);
	virtual void Render() override;
	virtual ~Cylinder() override;

private:
	void Generate(glm::vec3 scale);

private:
	glm::vec3 m_Scale;
};
