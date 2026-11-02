#pragma once

#include "Shader/MoravaShader.h"

#include <glm/glm.hpp>

#include <vector>


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
	void Update(glm::vec3 position, glm::vec3 scale);
	void Draw(H2M::RefH2M<MoravaShader> shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	virtual ~Pivot();
};
