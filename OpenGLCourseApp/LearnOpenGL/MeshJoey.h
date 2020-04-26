#pragma once

#include "glm/glm.hpp"

#include <string>

#include "../Shader.h"


struct Vertex
{    
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct TextureData
{
    unsigned int id;
    std::string type;
    std::string path;
};

class MeshJoey
{
public:
    MeshJoey();
    MeshJoey(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<TextureData> textures);
    void Draw(Shader* shader);
    inline unsigned int GetVAO() const { return VAO; };
    inline std::vector<unsigned int> GetIndices() const { return indices; };

private:
    void setupMesh();

private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureData> textures;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};
