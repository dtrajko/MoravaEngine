#include "QuadInstanced.h"

#include "GL/glew.h"


QuadInstanced::QuadInstanced()
{
	m_Positions = {
		// positions
		-0.5f, -0.5f, 0.0f, // bottom left
		 0.5f, -0.5f, 0.0f, // bottom right
		-0.5f,  0.5f, 0.0f, // top left
		 0.5f,  0.5f, 0.0f, // top right
	};

	m_TexCoord = {
		// texture Coords
		0.0f, 1.0f,   // bottom left
		1.0f, 1.0f,   // bottom right
		0.0f, 0.0f,   // top left
		1.0f, 0.0f,   // top right
	};

	// setup plane VAO
	// unsigned int m_VBO = 0;
	// glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	// glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STREAM_DRAW);
	// glBindBuffer(GL_ARRAY_BUFFER, 0);
	// glGenVertexArrays(1, &m_VAO);
	// glGenBuffers(1, &m_VBO);
	// glBindVertexArray(m_VAO);
	// glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	// glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	// glEnableVertexAttribArray(0);
	// glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	// glEnableVertexAttribArray(1);
	// glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

unsigned int QuadInstanced::CreateEmptyVBO(int floatCount)
{
	unsigned int VBO = 0;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, floatCount * 4, NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return VBO;
}

void QuadInstanced::LoadToVAO()
{
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);
	StoreDataInAttributeList(0, 3, &m_Positions[0]);
	StoreDataInAttributeList(1, 2, &m_TexCoord[0]);
}

void QuadInstanced::AddInstancedAttribute(int VAO, int VBO, int attribute, int dataSize, int instancedDataLength, int offset)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindVertexArray(VAO);
	glVertexAttribPointer(attribute, dataSize, GL_FLOAT, GL_FALSE, instancedDataLength * 4, (const void*)(size_t)(offset * 4));
	glVertexAttribDivisor(attribute, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void QuadInstanced::StoreDataInAttributeList(int attributeNumber, int coordinateSize, float* data)
{
	unsigned int VBO;
	glGenBuffers(1, &VBO);
	m_VBOs.push_back(VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, coordinateSize, data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(attributeNumber);
	glVertexAttribPointer(attributeNumber, coordinateSize, GL_FLOAT, false, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void QuadInstanced::UnbindVAO()
{
	glBindVertexArray(0);
}

void QuadInstanced::UpdateVBO(unsigned int VBO, int floatCount, float* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, floatCount * 4, data, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, floatCount * 4, data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void QuadInstanced::Render()
{
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

QuadInstanced::~QuadInstanced()
{
}
