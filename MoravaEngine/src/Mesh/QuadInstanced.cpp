#include "Mesh/QuadInstanced.h"

#include "GL/glew.h"

#include <cstdio>


QuadInstanced::QuadInstanced() : Quad()
{
	m_Positions = {
		// positions
		-0.5f, -0.5f, 0.0f, // bottom left
		 0.5f, -0.5f, 0.0f, // bottom right
		-0.5f,  0.5f, 0.0f, // top left
		 0.5f,  0.5f, 0.0f, // top right
	};

	m_VertexCount = 4;

	size_t verticesSize = m_Positions.size() * sizeof(float);
	unsigned int verticesStride = (unsigned int)verticesSize / m_VertexCount;

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesSize, &m_Positions[0], GL_STATIC_DRAW);
	// attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, verticesStride, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

unsigned int QuadInstanced::CreateEmptyVBO(int floatCount)
{
	glGenBuffers(1, &m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), NULL, GL_STREAM_DRAW);
	return m_VBO_Instanced;
}

void QuadInstanced::AddInstancedAttribute(int attribute, int dataSize, int instancedDataLength, int offset)
{
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glEnableVertexAttribArray(attribute);
	glVertexAttribPointer(attribute, dataSize, GL_FLOAT, GL_FALSE, instancedDataLength * sizeof(float), (const void*)(offset * sizeof(float)));
	glVertexAttribDivisor(attribute, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

unsigned int QuadInstanced::StoreDataInAttributeList(int attributeNumber, unsigned int floatCount, std::vector<float>* data)
{
	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), &data[0], GL_STREAM_DRAW);
	glEnableVertexAttribArray(attributeNumber);
	glVertexAttribPointer(attributeNumber, floatCount, GL_FLOAT, GL_FALSE, floatCount * sizeof(float), (const void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return VBO;
}

void QuadInstanced::UnbindVAO()
{
	glBindVertexArray(0);
}

void QuadInstanced::UpdateVBO(unsigned int floatCount, float* data)
{
	// printf("QuadInstanced::UpdateVBO buffer size: %zi\n", floatCount * sizeof(float));
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), data, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, floatCount * sizeof(float), data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void QuadInstanced::Render(unsigned int instanceCount)
{
	// printf("QuadInstanced::Render m_VertexCount = %u instanceCount = %u m_VAO = %u\n", m_VertexCount, instanceCount, m_VAO);
	glBindVertexArray(m_VAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, (size_t)m_VertexCount, (size_t)instanceCount);
	glBindVertexArray(0);

	// glBindVertexArray(m_VAO);
	// glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	// glBindVertexArray(0);
}

void QuadInstanced::Clear()
{
	Mesh::Clear();

	if (m_VBO_Instanced != 0)
	{
		glDeleteBuffers(1, &m_VBO_Instanced);
		m_VBO_Instanced = 0;
		printf("Mesh::Clear VBO_Instanced destroyed...\n");
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(5);
	glDisableVertexAttribArray(6);
}

QuadInstanced::~QuadInstanced()
{
	Clear();
}
