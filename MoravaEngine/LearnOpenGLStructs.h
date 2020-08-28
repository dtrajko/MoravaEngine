#pragma once

#include "glm/glm.hpp"

#include <string>


struct VertexSSAO
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct VertexTangents
{
    glm::vec3 Position;
    glm::vec2 TexCoord;
    glm::vec3 Normal;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;

};

struct TextureData
{
    unsigned int id;
    std::string type;
    std::string path;
};
