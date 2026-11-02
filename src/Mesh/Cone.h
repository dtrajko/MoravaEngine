#pragma once

#include "Mesh/Cylinder.h"

#include <glm/glm.hpp>


class Cone : public Cylinder
{
public:
	Cone();
	Cone(glm::vec3 scale);
	virtual void AdjustParameters(glm::vec3 scale) override;
	virtual ~Cone() override;

};
