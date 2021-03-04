#pragma once

#include "Shader/Shader.h"

#include <glm/glm.hpp>

#include <vector>


class Grid
{
public:
	Grid();
	Grid(int size);
	void Draw(Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	~Grid();

private:
	int m_Size;
	std::vector<float> m_Vertices;
	std::vector<unsigned int> m_Indices;

	unsigned int m_VAO;
	unsigned int m_VBO;
	unsigned int m_EBO;
};
