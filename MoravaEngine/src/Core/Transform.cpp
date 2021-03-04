#include "Core/Transform.h"


Transform::Transform()
{
	m_Position = glm::vec3(0.0f);
	m_Rotation = glm::vec3(0.0f);
	m_Scale = glm::vec3(1.0f);
}

Transform::Transform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	m_Position = position;
	m_Rotation = rotation;
	m_Scale = scale;
}

Transform::~Transform()
{
}
