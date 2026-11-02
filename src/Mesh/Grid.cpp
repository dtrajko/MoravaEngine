#include "Mesh/Grid.h"


Grid::Grid()
{
	m_Size = 1;
}

Grid::Grid(int size)
{
	m_Size = size;

    glm::vec4 color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);

    std::vector<float> vector;
    int index = 0;
    for (int z = -(size); z <= size; z++)
    {
        for (int x = -size; x <= size; x++)
        {
                vector = { (float)x, 0.0f, (float)z, color.r, color.g, color.b, color.a };
                m_Vertices.insert(m_Vertices.end(), std::begin(vector), std::end(vector));
                // printf("Grid vertices Index: %i, X=%.2ff Z=%.2ff\n", (int)index, (float)x, (float)z);
                index++;
        }
    }

    unsigned int lineCount = 2 * size + 1;
    std::vector<unsigned int> indexPair;

    // indices x direction
    for (unsigned int i = 0; i < lineCount; i++)
    {
        indexPair = { i * lineCount, i * lineCount + lineCount - 1 };
        m_Indices.insert(m_Indices.end(), std::begin(indexPair), std::end(indexPair));
        // printf("Grid indices %i, %i\n", indexPair[0], indexPair[1]);
    }

    // indices z direction
    for (unsigned int i = 0; i < lineCount; i++)
    {
        indexPair = { i, i + lineCount * (lineCount - 1) };
        m_Indices.insert(m_Indices.end(), std::begin(indexPair), std::end(indexPair));
        // printf("Grid indices %i, %i\n", indexPair[0], indexPair[1]);
    }

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(float), m_Vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), m_Indices.data(), GL_STATIC_DRAW);

    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);

    // Vertex Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
}

void Grid::Draw(H2M::RefH2M<MoravaShader> shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
    shader->Bind();
    glm::mat4 model = glm::mat4(1.0f);
    shader->SetMat4("model", model);
    shader->SetMat4("view", viewMatrix);
    shader->SetMat4("projection", projectionMatrix);

    glBindVertexArray(m_VAO);
    glDrawElements(GL_LINES, (GLsizei)m_Indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

Grid::~Grid()
{
    glDeleteBuffers(1, &m_EBO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteVertexArrays(1, &m_VAO);
}
