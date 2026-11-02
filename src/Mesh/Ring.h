#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/Model.h"


class Ring : public Mesh
{
public:
	Ring();
	Ring(glm::vec3 scale);
	virtual void Generate(glm::vec3 scale) override;
	virtual ~Ring() override;

private:
	Model* m_Model;

};
