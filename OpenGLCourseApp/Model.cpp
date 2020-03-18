#include "Model.h"

#include <chrono>



Model::Model()
{
	m_TexturesPath = "Textures";
}

void Model::LoadModel(const std::string& fileName, const std::string& texturesPath)
{
	printf("Loading model '%s'. Textures path '%s'\n", fileName.c_str(), texturesPath.c_str());

	m_TexturesPath = texturesPath;

	std::chrono::time_point<std::chrono::steady_clock> startTimepoint = std::chrono::high_resolution_clock::now();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName, 
		aiProcess_Triangulate | 
		aiProcess_FlipUVs |
		aiProcess_GenSmoothNormals | 
		aiProcess_JoinIdenticalVertices |
		aiProcess_CalcTangentSpace);

	if (!scene)
	{
		printf("Model '%s' failed to load: '%s'\n", fileName.c_str(), importer.GetErrorString());
		return;
	}

	auto endTimepoint = std::chrono::high_resolution_clock::now();
	long long start = std::chrono::time_point_cast<std::chrono::microseconds>(startTimepoint).time_since_epoch().count();
	long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();
	float duration = (end - start) * 0.000001f;
	printf("Model loaded in %.2f seconds.\n", duration);

	printf("Loading meshes...\n");

	LoadNode(scene->mRootNode, scene);

	printf("Loading materials...\n");

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
		// position
		vertices.insert(vertices.end(), { mesh->mVertices[i].x, mesh->mVertices[i].y , mesh->mVertices[i].z });

		// tex coords
		if (mesh->mTextureCoords[0])
		{
			vertices.insert(vertices.end(), { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y });
		}
		else
		{
			vertices.insert(vertices.end(), { 0.0f, 0.0f });
		}

		// normals
		if (mesh->mNormals)
		{
			vertices.insert(vertices.end(), { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z });
		}
		else
		{
			vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
		}

		// tangents
		if (mesh->mTangents)
		{
			vertices.insert(vertices.end(), { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z });
		}
		else
		{
			vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
		}

		// bitangents
		if (mesh->mBitangents)
		{
			vertices.insert(vertices.end(), { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z });
		}
		else
		{
			vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
		}
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	Mesh* newMesh = new Mesh();
	newMesh->CreateMesh(&vertices[0], &indices[0], (unsigned int)vertices.size(), (unsigned int)indices.size());
	meshList.push_back(newMesh);
	meshToTexture.push_back(mesh->mMaterialIndex);
}

void Model::LoadMaterials(const aiScene* scene)
{
	textureList.resize(scene->mNumMaterials);
	normalMapList.resize(scene->mNumMaterials);

	for (unsigned int i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];

		textureList[i] = nullptr;

		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			{
				size_t idx = std::string(path.data).rfind("\\");
				std::string filename = std::string(path.data).substr(idx + 1);

				std::string texPath = m_TexturesPath + std::string("/") + filename;

				printf("Texture loaded '%s'\n", texPath.c_str());

				textureList[i] = new Texture(texPath.c_str());

				if (!textureList[i]->Load())
				{
					printf("Failed to load texture at '%s'\n", texPath.c_str());
					delete textureList[i];
					textureList[i] = nullptr;
				}
			}
		}

		if (material->GetTextureCount(aiTextureType_HEIGHT))
		{
			aiString path;
			if (material->GetTexture(aiTextureType_HEIGHT, 0, &path) == AI_SUCCESS)
			{
				size_t idx = std::string(path.data).rfind("\\");
				std::string filename = std::string(path.data).substr(idx + 1);

				size_t idxBm = filename.rfind("-bm");
				filename = filename.substr(0, idxBm - 1);

				std::string texPath = m_TexturesPath + std::string("/") + filename;

				printf("Normal Map Texture loaded at '%s'\n", texPath.c_str());

				normalMapList[i] = new Texture(texPath.c_str());

				if (!normalMapList[i]->Load())
				{
					printf("Failed to load normal map at '%s'\n", texPath.c_str());
					delete normalMapList[i];
					normalMapList[i] = nullptr;
				}
			}
		}

		if (!textureList[i])
		{
			textureList[i] = new Texture("Textures/plain.png");
		}
	}
}

void Model::RenderModel(GLuint txSlotDiffuse, GLuint txSlotNormal, bool useNormalMaps)
{
	for (size_t i = 0; i < meshList.size(); i++)
	{
		unsigned int materialIndex = meshToTexture[i];

		if (materialIndex < textureList.size() && textureList[materialIndex])
		{
			textureList[materialIndex]->Bind(txSlotDiffuse);
		}

		if (useNormalMaps)
		{
			if (materialIndex < normalMapList.size() && normalMapList[materialIndex])
			{
				normalMapList[materialIndex]->Bind(txSlotNormal);
			}
		}

		meshList[i]->RenderMesh();
	}
}

void Model::RenderModelPBR()
{
	for (size_t i = 0; i < meshList.size(); i++)
	{
		meshList[i]->RenderMesh();
	}
}

void Model::ClearModel()
{
	for (size_t i = 0; i < meshList.size(); i++)
	{
		if (meshList[i])
		{
			delete meshList[i];
			meshList[i] = nullptr;
		}
	}

	for (size_t i = 0; i < textureList.size(); i++)
	{
		if (textureList[i] != nullptr)
		{
			delete textureList[i];
			textureList[i] = nullptr;
		}
	}

	meshList.clear();
	textureList.clear();
	meshToTexture.clear();
}

Model::~Model()
{
	ClearModel();
}
