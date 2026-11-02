#pragma once

#include "Mesh/Mesh.h"
#include "SongHo/CylinderSongHo.h"

#include <glm/glm.hpp>


class Cylinder : public Mesh
{
public:
	Cylinder();
	Cylinder(glm::vec3 scale);
	virtual void AdjustParameters(glm::vec3 scale);
	virtual void Generate(glm::vec3 scale) override;
	virtual ~Cylinder() override;

protected:
	float m_BaseRadius = 0.5f;
	float m_TopRadius = 0.5f;
	float m_Height = 1.0f;
	int m_Sectors = 24;
	int m_Stacks = 1;
	bool m_Smooth = false;

	float* m_Normals;
	float* m_TexCoords;

	CylinderSongHo* m_CylinderSH;

};
