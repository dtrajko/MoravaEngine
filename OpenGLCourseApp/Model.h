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
	Model(const std::string& fileName, const std::string& texturesPath = "Textures");
	void LoadModel(const std::string& fileName, const std::string& texturesPath = "Textures");
	void Update(glm::vec3 scale);
	inline const std::vector <Mesh*> GetMeshList() const { return meshList; };
	void Render(GLuint txSlotDiffuse, GLuint txSlotNormal, bool useNormalMaps);
	void RenderPBR();
	~Model();

private:
	void LoadNode(aiNode* node, const aiScene* scene);
	void LoadMesh(aiMesh* mesh, const aiScene* scene);
	void LoadMaterials(const aiScene* scene);
	void Clear();

private:
	std::vector <Mesh*> meshList;
	std::vector <Texture*> textureList;
	std::vector <Texture*> normalMapList;
	std::vector <unsigned int> meshToTexture;

	std::string m_TexturesPath;
	glm::vec3 m_Scale;

};
