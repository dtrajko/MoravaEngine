#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "Shader.h"


class Pivot
{
public:
	glm::vec3 m_Position = glm::vec3(0.0f);
	glm::vec3 m_Scale = glm::vec3(1.0f);

private:
	std::vector<float> m_Vertices;
	std::vector<unsigned int> m_Indices;

public:
	Pivot(glm::vec3 position, glm::vec3 scale);
	void UpdatePosition(glm::vec3 position);
	void Draw(Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	virtual ~Pivot();
};
