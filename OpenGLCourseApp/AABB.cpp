#include "AABB.h"


AABB::AABB(glm::vec3 position, glm::vec3 scale)
{
	m_Position = position;
	m_Scale = scale;
}

void AABB::UpdatePosition(glm::vec3 position)
{
    m_Position = position;
}

bool AABB::Contains(glm::vec3 position, glm::vec3 scale)
{
	bool contains = !(
		m_Position.x + m_Scale.x < position.x || position.x + scale.x < m_Position.x ||
		m_Position.y + m_Scale.y < position.y || position.y + scale.y < m_Position.y ||
		m_Position.z + m_Scale.z < position.z || position.z + scale.z < m_Position.z);
	return contains;
}

/*
	* https://www.toptal.com/game/video-game-physics-part-ii-collision-detection-for-solid-objects
	*/
bool AABB::TestAABBOverlap(AABB * a, AABB * b)
{
	float d1x = b->m_Position.x - a->m_Position.x + a->m_Scale.x;
	float d1y = b->m_Position.y - a->m_Position.y + a->m_Scale.y;
	float d1z = b->m_Position.z - a->m_Position.z + a->m_Scale.z;
	float d2x = a->m_Position.x - b->m_Position.x + b->m_Scale.x;
	float d2y = a->m_Position.y - b->m_Position.y + b->m_Scale.y;
	float d2z = a->m_Position.z - b->m_Position.z + b->m_Scale.z;

	if (d1x > 0.0f || d1y > 0.0f || d1z > 0.0f)
		return false;

	if (d2x > 0.0f || d2y > 0.0f || d2z > 0.0f)
		return false;

	return true;
}

void AABB::Draw(Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
    glm::vec4 color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    float offset = 0.02f;
    float sizeX = (0.5f + offset) * m_Scale.x;
    float sizeY = (0.5f + offset) * m_Scale.y;
    float sizeZ = (0.5f + offset) * m_Scale.z;

    m_Vertices = {
        m_Position.x - sizeX, m_Position.y - sizeY, m_Position.z + sizeZ,    color.r, color.g, color.b, color.a,
        m_Position.x + sizeX, m_Position.y - sizeY, m_Position.z + sizeZ,    color.r, color.g, color.b, color.a,
        m_Position.x + sizeX, m_Position.y + sizeY, m_Position.z + sizeZ,    color.r, color.g, color.b, color.a,
        m_Position.x - sizeX, m_Position.y + sizeY, m_Position.z + sizeZ,    color.r, color.g, color.b, color.a,
        m_Position.x - sizeX, m_Position.y - sizeY, m_Position.z - sizeZ,    color.r, color.g, color.b, color.a,
        m_Position.x + sizeX, m_Position.y - sizeY, m_Position.z - sizeZ,    color.r, color.g, color.b, color.a,
        m_Position.x + sizeX, m_Position.y + sizeY, m_Position.z - sizeZ,    color.r, color.g, color.b, color.a,
        m_Position.x - sizeX, m_Position.y + sizeY, m_Position.z - sizeZ,    color.r, color.g, color.b, color.a,
    };

    m_Indices = {
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        4, 5,
        5, 6,
        6, 7,
        7, 4,
        0, 4,
        1, 5,
        2, 6,
        3, 7,
    };

    unsigned int m_LineVAO;
    unsigned int m_LineVBO;
    unsigned int m_LineEBO;

    glGenVertexArrays(1, &m_LineVAO);
    glGenBuffers(1, &m_LineVBO);
    glGenBuffers(1, &m_LineEBO);

    glBindVertexArray(m_LineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(float), m_Vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LineEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), m_Indices.data(), GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);

    // Vertex Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    shader->Bind();
    glm::mat4 model = glm::mat4(1.0f);
    shader->setMat4("model", model);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);

    glBindVertexArray(m_LineVAO);
    glDrawElements(GL_LINES, (GLsizei)m_Indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDeleteBuffers(1, &m_LineEBO);
    glDeleteBuffers(1, &m_LineVBO);
    glDeleteVertexArrays(1, &m_LineVAO);
}

AABB::~AABB()
{

}
