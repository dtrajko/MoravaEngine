#pragma once


#include "Mesh.h"


class Ring : public Mesh
{
public:
	Ring();
	Ring(glm::vec3 scale);
	virtual void Generate(glm::vec3 scale) override;
	virtual ~Ring() override;

private:
	void ComputeNormals();

private:
	int m_Slices = 24;
	float m_Tau = 6.28318530718f;

	glm::vec3 m_AxisX = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 m_AxisY = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 m_AxisZ = glm::vec3(0.0f, 0.0f, 1.0f);
};
