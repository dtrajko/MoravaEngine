#pragma once

#include "LearnOpenGL/LearnOpenGLStructs.h"
#include "Shader/Shader.h"

#include <string>


/**
 * Mesh class compatible with LearnOpenGL code / shaders
 * Vertex buffer structure as follows:
 *
 * layout (location = 0) in vec3 aPos;
 * layout (location = 1) in vec3 aNormal;
 * layout (location = 2) in vec2 aTexCoords;
 */
class MeshSSAO
{
public:
    MeshSSAO();
    MeshSSAO(std::vector<VertexSSAO> vertices, std::vector<unsigned int> indices, std::vector<TextureData> textures);
    void Draw(Shader* shader);
    inline unsigned int GetVAO() const { return VAO; };
    inline std::vector<unsigned int> GetIndices() const { return indices; };

protected:
    virtual void setupMesh();

protected:
    std::vector<unsigned int> indices;
    std::vector<TextureData> textures;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

private:
    std::vector<VertexSSAO> vertices;

};
