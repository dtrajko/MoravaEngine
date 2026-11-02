#include "Mesh/QuadSSAO.h"

#include "GL/glew.h"


QuadSSAO::QuadSSAO()
{
	float vertices[] = {
		// Positions           // TexCoords
		-1.0f,  1.0f, 0.0f,    0.0f, 1.0f,    // bottom left
		-1.0f, -1.0f, 0.0f,    0.0f, 0.0f,    // bottom right
		 1.0f,  1.0f, 0.0f,    1.0f, 1.0f,    // top left
		 1.0f, -1.0f, 0.0f,    1.0f, 0.0f,    // top right
	};

	// setup plane VAO
	unsigned int m_VBO = 0;

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	// fill buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

	// link vertex attributes
	glBindVertexArray(m_VAO);

	// layout (location = 0) in vec3 aPos;
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

	// layout(location = 1) in vec2 aTexCoords;
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void QuadSSAO::Render()
{
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

QuadSSAO::~QuadSSAO()
{
}
