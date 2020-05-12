#pragma once

#include <glm/glm.hpp>

#include "Cylinder.h"


class Cone : public Cylinder
{
public:
	Cone();
	Cone(glm::vec3 scale);
	virtual void AdjustParameters(glm::vec3 scale) override;
	virtual ~Cone() override;

};
