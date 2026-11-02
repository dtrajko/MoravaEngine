#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/Model.h"


class Ring90 : public Mesh
{
public:
	Ring90();
	Ring90(glm::vec3 scale);
	virtual void Generate(glm::vec3 scale) override;
	virtual ~Ring90() override;

private:
	Model* m_Model;

};
