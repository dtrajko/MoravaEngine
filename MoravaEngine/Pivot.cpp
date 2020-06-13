#include "Pivot.h"

Pivot::Pivot(glm::vec3 position, glm::vec3 scale)
{
	m_Position = position;
	m_Scale = scale;
}

void Pivot::UpdatePosition(glm::vec3 position)
{
    m_Position = position;
}

void Pivot::Update(glm::vec3 position, glm::vec3 scale)
{
    m_Position = position;
    m_Scale = scale;
}

void Pivot::Draw(Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
    glm::vec4 colorX = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 colorY = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec4 colorZ = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    float sizeX = 0.5f * m_Scale.x;
    float sizeY = 0.5f * m_Scale.y;
    float sizeZ = 0.5f * m_Scale.z;

    m_Vertices = {
        m_Position.x,         m_Position.y,         m_Position.z,            colorX.r, colorX.g, colorX.b, colorX.a,
        m_Position.x + sizeX, m_Position.y,         m_Position.z,            colorX.r, colorX.g, colorX.b, colorX.a,
        m_Position.x,         m_Position.y,         m_Position.z,            colorY.r, colorY.g, colorY.b, colorY.a,
        m_Position.x,         m_Position.y + sizeY, m_Position.z,            colorY.r, colorY.g, colorY.b, colorY.a,
        m_Position.x,         m_Position.y,         m_Position.z,            colorZ.r, colorZ.g, colorZ.b, colorZ.a,
        m_Position.x,         m_Position.y,         m_Position.z + sizeZ,    colorZ.r, colorZ.g, colorZ.b, colorZ.a,
    };

    m_Indices = {
        0, 1,
        2, 3,
        4, 5,
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

Pivot::~Pivot()
{
}
