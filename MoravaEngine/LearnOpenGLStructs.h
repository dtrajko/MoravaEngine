#pragma once

#include "glm/glm.hpp"


struct VertexSSAO
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct VertexTangents
{
    VertexSSAO base;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct TextureData
{
    unsigned int id;
    std::string type;
    std::string path;
};
