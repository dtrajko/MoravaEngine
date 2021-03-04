#include "Core/Raycast.h"

#include <GL/glew.h>


Raycast::Raycast()
{
}

void Raycast::Draw(glm::vec3 start, glm::vec3 end, glm::vec4 color, Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
    m_LineStart = start;
    m_LineEnd = end;
    m_Color = color;
    m_Color = m_Hit ? m_ColorHitTrue : m_ColorHitFalse;

    float lineVertices[] =
    {
        // position                                     // color
        m_LineStart.x, m_LineStart.y, m_LineStart.z,    m_Color.r, m_Color.g, m_Color.b, m_Color.a,
        m_LineEnd.x,   m_LineEnd.y,   m_LineEnd.z,      m_Color.r, m_Color.g, m_Color.b, m_Color.a,
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

    glDeleteVertexArrays(1, &m_LineVAO);
    glDeleteBuffers(1, &m_LineVBO);
}

Raycast::~Raycast()
{
}
