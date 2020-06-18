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

	m_VertexCount = 4;
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
	StoreDataInAttributeList(0, 3, &m_Positions);
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

void QuadInstanced::StoreDataInAttributeList(int attributeNumber, int coordinateSize, std::vector<float>* data)
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

void QuadInstanced::UpdateVBO(unsigned int VBO, unsigned int floatCount, std::vector<float>* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), &data[0], GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, floatCount * sizeof(float), &data[0]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void QuadInstanced::Render(unsigned int instanceCount)
{
	glBindVertexArray(m_VAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, (size_t)m_VertexCount, (size_t)instanceCount);
	glBindVertexArray(0);
}

QuadInstanced::~QuadInstanced()
{
}
