#include "Mesh/HazelFullscreenQuad.h"

#include "GL/glew.h"


HazelFullscreenQuad::HazelFullscreenQuad()
{
	SetupFullscreenQuad();
}

void HazelFullscreenQuad::SetupFullscreenQuad()
{
	// Create fullscreen quad
	float x = -1;
	float y = -1;
	float width = 2;
	float height = 2;

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;
	};

	QuadVertex* data = new QuadVertex[4];

	data[0].Position = glm::vec3(x, y, 0.1f);
	data[0].TexCoord = glm::vec2(0, 0);

	data[1].Position = glm::vec3(x + width, y, 0.1f);
	data[1].TexCoord = glm::vec2(1, 0);

	data[2].Position = glm::vec3(x + width, y + height, 0.1f);
	data[2].TexCoord = glm::vec2(1, 1);

	data[3].Position = glm::vec3(x, y + height, 0.1f);
	data[3].TexCoord = glm::vec2(0, 1);

	uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };

	glGenVertexArrays(1, &m_FullscreenQuadVAO);
	// link vertex attributes
	glBindVertexArray(m_FullscreenQuadVAO);

	glGenBuffers(1, &m_FullscreenQuadVBO);

	// fill buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_FullscreenQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * 4, &data[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_FullscreenQuadIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_FullscreenQuadIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * 6, &indices[0], GL_STATIC_DRAW);

	// position layout (location = 0) in vec3 aPos;
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void*)offsetof(QuadVertex, Position));

	// tex coord layout(location = 1) in vec2 aTexCoords;
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void*)offsetof(QuadVertex, TexCoord));

	glBindBuffer(GL_ARRAY_BUFFER, 0);         // Unbind VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

void HazelFullscreenQuad::Render()
{
	glBindVertexArray(m_FullscreenQuadVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_FullscreenQuadIBO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
	glBindVertexArray(0);                     // Unbind VAO
}

HazelFullscreenQuad::~HazelFullscreenQuad()
{
	glDeleteVertexArrays(1, &m_FullscreenQuadVAO);
	glDeleteBuffers(1, &m_FullscreenQuadVBO);
	glDeleteBuffers(1, &m_FullscreenQuadIBO);
}
