#pragma once

#include <glm/glm.hpp>

#include "Mesh.h"
#include "SongHo/CylinderSongHo.h"


class Cylinder : public Mesh
{
public:
	Cylinder();
	Cylinder(glm::vec3 scale);
	void Generate(glm::vec3 scale);
	virtual void Update(glm::vec3 scale) override;
	virtual ~Cylinder() override;

private:
	glm::vec3 m_Scale;

	float m_BaseRadius = 0.5f;
	float m_TopRadius = 0.5f;
	float m_Height = 1.0f;
	int m_Sectors = 12;
	int m_Stacks = 1;
	bool m_Smooth = false;

	float* m_Normals;
	float* m_TexCoords;

	CylinderSongHo* m_CylinderSH;

};
