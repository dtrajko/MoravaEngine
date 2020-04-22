#pragma once

#include <assimp/scene.h>

#include "MeshJoey.h"


class ModelJoey
{
public:
    ModelJoey(std::string const& path, bool gamma = false);
    void Draw(Shader* shader);
    inline std::vector<TextureData> GetTextures() { return textures_loaded; };
    inline std::vector<MeshJoey> GetMeshes() { return meshes; };

private:
    void loadModel(std::string const& path);
    void processNode(aiNode* node, const aiScene* scene);
    MeshJoey processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<TextureData> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

private:
    std::vector<TextureData> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    std::vector<MeshJoey> meshes;
    std::string directory;
    bool gammaCorrection;

};
