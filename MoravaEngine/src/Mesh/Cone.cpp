#include "Mesh/Cone.h"


Cone::Cone() : Cone(glm::vec3(1.0f))
{
	m_TopRadius = 0;
}

Cone::Cone(glm::vec3 scale)
{
	Mesh();

	m_TopRadius = 0;

	Generate(scale);
}

void Cone::AdjustParameters(glm::vec3 scale)
{
	float newRadius = m_BaseRadius;

	if (scale != m_Scale)
	{
		newRadius = scale.x / 2.0f;
		if (scale.z > scale.x) newRadius = scale.z / 2.0f;

		m_BaseRadius = newRadius;
		m_TopRadius = 0;

		m_Height = scale.y;
		m_Stacks = (int)scale.y;
	}
}

Cone::~Cone()
{
}
