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

	m_VertexCount = 4;

	/* CreateEmptyVBO(INSTANCE_DATA_LENGTH * MAX_INSTANCES); */
	glGenBuffers(1, &m_VBO_Instanced);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBufferData(GL_ARRAY_BUFFER, 21 * 10000 * sizeof(float), NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* static_cast<QuadInstanced*>(m_Mesh)->LoadToVAO(); */
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	/* StoreDataInAttributeList(0, 3, &m_Positions); */
	unsigned int VBO_Positions;
	glGenBuffers(1, &VBO_Positions); // !!!!!!!!!!!!!!!!
	m_VBOs.push_back(VBO_Positions); // !!!!!!!!!!!!!!!!
	glBindBuffer(GL_ARRAY_BUFFER, VBO_Positions);
	glBufferData(GL_ARRAY_BUFFER, 3, &m_Positions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* StoreDataInAttributeList(1, 2, &m_TexCoord); */
	unsigned int VBO_TexCoord;
	glGenBuffers(1, &VBO_TexCoord);
	m_VBOs.push_back(VBO_TexCoord);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_TexCoord);
	glBufferData(GL_ARRAY_BUFFER, 2, &m_TexCoord, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(m_Mesh->GetVAO(), VBO_Instanced, 2, 4, INSTANCE_DATA_LENGTH,  0); */
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 21 * sizeof(float), (const void*)(0 * sizeof(float)));
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	/* static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(m_Mesh->GetVAO(), VBO_Instanced, 3, 4, INSTANCE_DATA_LENGTH,  4); */
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 21 * sizeof(float), (const void*)(4 * sizeof(float)));
	glVertexAttribDivisor(3, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	/* static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(m_Mesh->GetVAO(), VBO_Instanced, 4, 4, INSTANCE_DATA_LENGTH,  8); */
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 21 * sizeof(float), (const void*)(8 * sizeof(float)));
	glVertexAttribDivisor(4, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	/* static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(m_Mesh->GetVAO(), VBO_Instanced, 5, 4, INSTANCE_DATA_LENGTH, 12); */
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 21 * sizeof(float), (const void*)(12 * sizeof(float)));
	glVertexAttribDivisor(5, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	/* static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(m_Mesh->GetVAO(), VBO_Instanced, 6, 4, INSTANCE_DATA_LENGTH, 16); */
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 21 * sizeof(float), (const void*)(16 * sizeof(float)));
	glVertexAttribDivisor(6, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	/* static_cast<QuadInstanced*>(m_Mesh)->AddInstancedAttribute(m_Mesh->GetVAO(), VBO_Instanced, 7, 1, INSTANCE_DATA_LENGTH, 20); */
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Instanced);
	glBindVertexArray(m_VAO);
	glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 21 * sizeof(float), (const void*)(20 * sizeof(float)));
	glVertexAttribDivisor(7, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

unsigned int QuadInstanced::CreateEmptyVBO(int floatCount)
{
	unsigned int VBO = 0;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return VBO;
}

void QuadInstanced::LoadToVAO()
{
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);
	StoreDataInAttributeList(0, 3, &m_Positions);
	StoreDataInAttributeList(1, 2, &m_TexCoord);
}

void QuadInstanced::AddInstancedAttribute(int VAO, int VBO, int attribute, int dataSize, int instancedDataLength, int offset)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindVertexArray(VAO);
	glVertexAttribPointer(attribute, dataSize, GL_FLOAT, GL_FALSE, instancedDataLength * sizeof(float), (const void*)(offset * sizeof(float)));
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
	// printf("QuadInstanced::Render m_VertexCount = %u instanceCount = %u m_VAO = %u\n", m_VertexCount, instanceCount, m_VAO);
	glBindVertexArray(m_VAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, (size_t)m_VertexCount, (size_t)instanceCount);
	glBindVertexArray(0);
}

QuadInstanced::~QuadInstanced()
{
}
