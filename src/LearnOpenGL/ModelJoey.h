#pragma once

#include <assimp/scene.h>

#include "MeshJoey.h"
#include "ModelSSAO.h"


/**
 * Model class compatible with LearnOpenGL code / shaders
 * Vertex buffer structure as follows:
 *
 * layout (location = 0) in vec3 aPosition;
 * layout (location = 1) in vec2 aTexCoord;
 * layout (location = 2) in vec3 aNormal;
 * layout (location = 3) in vec3 aTangent;
 * layout (location = 4) in vec3 aBitangent;
 */
class ModelJoey : public ModelSSAO
{
public:
    ModelJoey();
    ModelJoey(std::string const& path, std::string const& textureDirectory = "", bool gamma = false);
    virtual ~ModelJoey();

    virtual void Draw(H2M::RefH2M<MoravaShader> shader) override;
    inline std::vector<MeshJoey> GetMeshes() { return meshes; };

protected:
    virtual void loadModel(std::string const& path) override;
    virtual void processNode(aiNode* node, const aiScene* scene) override;

    MeshJoey processMesh(aiMesh* mesh, const aiScene* scene);

private:
    std::vector<MeshJoey> meshes;

};
