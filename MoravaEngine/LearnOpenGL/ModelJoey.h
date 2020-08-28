#pragma once

#include <assimp/scene.h>

#include "MeshJoey.h"


/**
 * Model class compatible with LearnOpenGL code / shaders
 * Vertex buffer structure as follows:
 * layout (location = 0) in vec3 aPosition;
 * layout (location = 1) in vec2 aTexCoord;
 * layout (location = 2) in vec3 aNormal;
 * layout (location = 3) in vec3 aTangent;
 * layout (location = 4) in vec3 aBitangent;
 */
class ModelJoey
{
public:
    ModelJoey(std::string const& path, std::string const& textureDirectory = "", bool gamma = false);
    void Draw(Shader* shader);
    inline std::vector<TextureData> GetTextures() { return textures_loaded; };
    inline std::vector<MeshJoey> GetMeshes() { return meshes; };

private:
    void loadModel(std::string const& path);
    void processNode(aiNode* node, const aiScene* scene);
    MeshJoey processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<TextureData> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

private:
    std::vector<MeshJoey> meshes;
    std::vector<TextureData> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    std::string m_ModelDirectory;
    std::string m_TextureDirectory;
    bool gammaCorrection;

};
