#include "Quad.h"

#include "GL/glew.h"

#include <vector>


Quad::Quad()
{
	std::vector<float> vertices = {
		// positions
		-0.5f, -0.5f, 0.0f, // bottom left
		 0.5f, -0.5f, 0.0f, // bottom right
		-0.5f,  0.5f, 0.0f, // top left
		 0.5f,  0.5f, 0.0f, // top right
	};

	m_VertexCount = 4;

	size_t verticesSize = vertices.size() * sizeof(float);
	unsigned int verticesStride = (unsigned int)verticesSize / m_VertexCount;

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesSize, &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, verticesStride, (void*)0);
}

void Quad::Render()
{
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

Quad::~Quad()
{
}
