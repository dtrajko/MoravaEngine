#pragma once

#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Texture.h"


class Model
{
public:
	Model();
	void LoadModel(const std::string& fileName);
	void RenderModel();
	void ClearModel();
	~Model();

private:
	void LoadNode(aiNode* node, const aiScene* scene);
	void LoadMesh(aiMesh* mesh, const aiScene* scene);
	void LoadMaterials(const aiScene* scene);

private:
	std::vector <Mesh*> meshList;
	std::vector <Texture*> textureList;
	std::vector <Texture*> normalMapList;
	std::vector <unsigned int> meshToTexture;


};
