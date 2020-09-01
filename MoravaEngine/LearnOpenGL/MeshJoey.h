#pragma once

#include <string>

#include "../Shader.h"
#include "../LearnOpenGLStructs.h"
#include "MeshSSAO.h"


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
class MeshJoey : public MeshSSAO
{
public:
    MeshJoey();
    MeshJoey(std::vector<VertexTangents> vertices, std::vector<unsigned int> indices, std::vector<TextureData> textures);

private:
    virtual void setupMesh() override;

private:
    std::vector<VertexTangents> vertices;

};
