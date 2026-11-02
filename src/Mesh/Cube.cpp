#include "Mesh/Cube.h"

#include "LearnOpenGL/LearnOpenGLStructs.h"

#include "GL/glew.h"


Cube::Cube()
{
	// initialize (if necessary)
	float vertices[] = {
		// Position XYZ         // Normal XYZ          // TexCoords UV
		// back face
		-1.0f, -1.0f, -1.0f,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f, // bottom-left
		 1.0f,  1.0f, -1.0f,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f, // top-right
		 1.0f, -1.0f, -1.0f,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f, // bottom-right         
		 1.0f,  1.0f, -1.0f,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f, // top-right
		-1.0f, -1.0f, -1.0f,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f, // bottom-left
		-1.0f,  1.0f, -1.0f,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f, // top-left
		// front face
		-1.0f, -1.0f,  1.0f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f, // top-right
		 1.0f,  1.0f,  1.0f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f, // top-right
		-1.0f,  1.0f,  1.0f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f, // top-left
		-1.0f, -1.0f,  1.0f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f, // bottom-left
		// left face
		-1.0f,  1.0f,  1.0f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // top-right
		-1.0f,  1.0f, -1.0f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // top-left
		-1.0f, -1.0f, -1.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f, -1.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f,  1.0f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // bottom-right
		-1.0f,  1.0f,  1.0f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // top-right
		// right face
		 1.0f,  1.0f,  1.0f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // top-left
		 1.0f, -1.0f, -1.0f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // top-right         
		 1.0f, -1.0f, -1.0f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // top-left
		 1.0f, -1.0f,  1.0f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // bottom-left     
		// bottom face
		-1.0f, -1.0f, -1.0f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f, // top-right
		 1.0f, -1.0f, -1.0f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f, // top-left
		 1.0f, -1.0f,  1.0f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f, // bottom-left
		-1.0f, -1.0f,  1.0f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f, // bottom-right
		-1.0f, -1.0f, -1.0f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f, // top-right
		// top face
		-1.0f,  1.0f, -1.0f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, // top-left
		 1.0f,  1.0f , 1.0f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f, // top-right     
		 1.0f,  1.0f,  1.0f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f, // bottom-right
		-1.0f,  1.0f, -1.0f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, // top-left
		-1.0f,  1.0f,  1.0f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,  // bottom-left        
	};

	unsigned int m_VBO = 0;

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	// fill buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// link vertex attributes
	glBindVertexArray(m_VAO);

	// layout (location = 0) in vec3 aPos;
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexSSAO), (void*)offsetof(VertexSSAO, Position));

	// layout (location = 1) in vec3 aNormal;
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexSSAO), (void*)offsetof(VertexSSAO, Normal));

	// layout(location = 2) in vec2 aTexCoords;
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexSSAO), (void*)offsetof(VertexSSAO, TexCoords));

	// release buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Cube::Render()
{
	// render Cube
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

Cube::~Cube()
{
}
