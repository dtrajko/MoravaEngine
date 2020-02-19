#include "Model.h"



Model::Model()
{
}

void Model::LoadModel(const std::string& fileName)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName, 
		aiProcess_Triangulate | 
		aiProcess_FlipUVs | 
		aiProcess_GenSmoothNormals | 
		aiProcess_JoinIdenticalVertices);

	if (!scene)
	{
		printf("Model '%s' failed to load: '%s'", fileName, importer.GetErrorString());
		return;
	}

	LoadNode(scene->mRootNode, scene);

	LoadMaterials(scene);
}

void Model::LoadNode(aiNode* node, const aiScene* scene)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		LoadMesh(scene->mMeshes[node->mMeshes[i]], scene);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		LoadNode(node->mChildren[i], scene);
	}
}

void Model::LoadMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<GLfloat> vertices;
	std::vector<unsigned int> indices;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		vertices.insert(vertices.end(), { mesh->mVertices[i].x, mesh->mVertices[i].y , mesh->mVertices[i].z });
		if (mesh->mTextureCoords[0])
		{
			vertices.insert(vertices.end(), { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y });
		}
		else
		{
			vertices.insert(vertices.end(), { 0.0f, 0.0f });
		}
		vertices.insert(vertices.end(), { mesh->mNormals[i].x, mesh->mNormals[i].y , mesh->mNormals[i].z });
	}
}

void Model::LoadMaterials(const aiScene* scene)
{
}

void Model::RenderModel()
{
}

void Model::ClearModel()
{
}

Model::~Model()
{
}
