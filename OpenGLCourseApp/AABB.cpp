#include "AABB.h"


AABB::AABB(glm::vec3 position, glm::vec3 scale)
{
	m_Position = position;
	m_Scale = scale;
}

bool AABB::contains(glm::vec3 position, glm::vec3 scale)
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
bool AABB::testAABBOverlap(AABB * a, AABB * b)
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

void AABB::UpdatePosition(glm::vec3 position)
{
	m_Position = position;
}

void AABB::Draw(Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
    glm::vec4 color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    float offset = 0.05f;
    float size = 0.5f + offset;

    // Front face
    DrawLine(glm::vec3(m_Position.x - size, m_Position.y - size, m_Position.z + size),
             glm::vec3(m_Position.x + size, m_Position.y - size, m_Position.z + size), color, shader, projectionMatrix, viewMatrix);

    DrawLine(glm::vec3(m_Position.x - size, m_Position.y + size, m_Position.z + size),
             glm::vec3(m_Position.x + size, m_Position.y + size, m_Position.z + size), color, shader, projectionMatrix, viewMatrix);

    DrawLine(glm::vec3(m_Position.x - size, m_Position.y - size, m_Position.z + size),
             glm::vec3(m_Position.x - size, m_Position.y + size, m_Position.z + size), color, shader, projectionMatrix, viewMatrix);

    DrawLine(glm::vec3(m_Position.x + size, m_Position.y - size, m_Position.z + size),
             glm::vec3(m_Position.x + size, m_Position.y + size, m_Position.z + size), color, shader, projectionMatrix, viewMatrix);

    // Back face
    DrawLine(glm::vec3(m_Position.x - size, m_Position.y - size, m_Position.z - size),
             glm::vec3(m_Position.x + size, m_Position.y - size, m_Position.z - size), color, shader, projectionMatrix, viewMatrix);

    DrawLine(glm::vec3(m_Position.x - size, m_Position.y + size, m_Position.z - size),
             glm::vec3(m_Position.x + size, m_Position.y + size, m_Position.z - size), color, shader, projectionMatrix, viewMatrix);

    DrawLine(glm::vec3(m_Position.x - size, m_Position.y - size, m_Position.z - size),
             glm::vec3(m_Position.x - size, m_Position.y + size, m_Position.z - size), color, shader, projectionMatrix, viewMatrix);

    DrawLine(glm::vec3(m_Position.x + size, m_Position.y - size, m_Position.z - size),
             glm::vec3(m_Position.x + size, m_Position.y + size, m_Position.z - size), color, shader, projectionMatrix, viewMatrix);

    // Side lines
    DrawLine(glm::vec3(m_Position.x - size, m_Position.y - size, m_Position.z + size),
             glm::vec3(m_Position.x - size, m_Position.y - size, m_Position.z - size), color, shader, projectionMatrix, viewMatrix);

    DrawLine(glm::vec3(m_Position.x + size, m_Position.y - size, m_Position.z + size),
             glm::vec3(m_Position.x + size, m_Position.y - size, m_Position.z - size), color, shader, projectionMatrix, viewMatrix);

    DrawLine(glm::vec3(m_Position.x - size, m_Position.y + size, m_Position.z + size),
             glm::vec3(m_Position.x - size, m_Position.y + size, m_Position.z - size), color, shader, projectionMatrix, viewMatrix);

    DrawLine(glm::vec3(m_Position.x + size, m_Position.y + size, m_Position.z + size),
             glm::vec3(m_Position.x + size, m_Position.y + size, m_Position.z - size), color, shader, projectionMatrix, viewMatrix);

}

void AABB::DrawLine(glm::vec3 start, glm::vec3 end, glm::vec4 color, Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
    float lineVertices[] =
    {
        // position                   // color
        start.x, start.y, start.z,    color.r, color.g, color.b, color.a,
        end.x,   end.y,   end.z,      color.r, color.g, color.b, color.a,
    };

    // line VAO
    unsigned int m_LineVAO;
    unsigned int m_LineVBO;
    glGenVertexArrays(1, &m_LineVAO);
    glGenBuffers(1, &m_LineVBO);
    glBindVertexArray(m_LineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), &lineVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));

    // line
    shader->Bind();
    glm::mat4 model = glm::mat4(1.0f);
    shader->setMat4("model", model);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);

    glBindVertexArray(m_LineVAO);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

AABB::~AABB()
{

}
