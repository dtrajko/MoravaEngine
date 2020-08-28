#pragma once

#include <string>

#include "../Shader.h"
#include "../LearnOpenGLStructs.h"


class MeshJoey
{
public:
    MeshJoey();
    MeshJoey(std::vector<VertexTangents> vertices, std::vector<unsigned int> indices, std::vector<TextureData> textures);
    void Draw(Shader* shader);
    inline unsigned int GetVAO() const { return VAO; };
    inline std::vector<unsigned int> GetIndices() const { return indices; };

private:
    void setupMesh();

private:
    std::vector<VertexTangents> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureData> textures;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};
