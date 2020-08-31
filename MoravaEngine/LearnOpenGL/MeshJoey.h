#pragma once

#include <string>

#include "../Shader.h"
#include "../LearnOpenGLStructs.h"


/**
 * Mesh class compatible with LearnOpenGL code / shaders
 * Vertex buffer structure as follows:
 *
 * layout (location = 0) in vec3 aPosition;
 * layout (location = 1) in vec2 aTexCoord;
 * layout (location = 2) in vec3 aNormal;
 * layout (location = 3) in vec3 aTangent;
 * layout (location = 4) in vec3 aBitangent;
 */
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
