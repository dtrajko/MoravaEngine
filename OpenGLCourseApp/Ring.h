#pragma once

#include "Mesh.h"
#include "Model.h"


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
