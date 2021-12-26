#include "Mesh/Model.h"

#include "Core/Log.h"
#include "Texture/TextureLoader.h"

#include <chrono>


Model::Model()
{
	m_TexturesPath = "Textures";
	m_Scale = glm::vec3(1.0f);
}

Model::Model(const std::string& fileName, const std::string& texturesPath)
{
	m_TexturesPath = (texturesPath != "") ? texturesPath : "Textures";
	LoadModel(fileName, m_TexturesPath);
}

void Model::LoadModel(const std::string& fileName, const std::string& texturesPath)
{
	Log::GetLogger()->info("Loading model '{0}'. Textures path '{1}'", fileName, texturesPath);

	m_TexturesPath = (texturesPath != "") ? texturesPath : "Textures";
	m_Scale = glm::vec3(1.0f);

	std::chrono::time_point<std::chrono::high_resolution_clock> startTimepoint = std::chrono::high_resolution_clock::now();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName, 
		aiProcess_Triangulate | 
		aiProcess_FlipUVs |
		aiProcess_GenSmoothNormals | 
		aiProcess_JoinIdenticalVertices |
		aiProcess_CalcTangentSpace);

	if (!scene)
	{
		Log::GetLogger()->error("Model '{0}' failed to load: '{1}'", fileName, importer.GetErrorString());
		return;
	}

	auto endTimepoint = std::chrono::high_resolution_clock::now();
	long long start = std::chrono::time_point_cast<std::chrono::microseconds>(startTimepoint).time_since_epoch().count();
	long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();
	float duration = (end - start) * 0.000001f;
	Log::GetLogger()->info("Model loaded in {0} seconds.", duration);

	Log::GetLogger()->info("Loading meshes...");

	LoadNode(scene->mRootNode, scene);

	Log::GetLogger()->info("Loading materials...");

	LoadMaterials(scene);
}

/*
 * Recursive method for loading all nodes (meshes) from the ASSIMP tree data structure
 */
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

/*
 * Process and convert ASSIMP aiMesh data structure to our internal Mesh class
 * Mesh data structure contains of Vertex Position, Normals, Texture Coordinates,
 * Tangents, BiTangents and the texture tilingFactor
 * It also processes Indices from Mesh Faces
 */
void Model::LoadMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<GLfloat> vertices;
	std::vector<unsigned int> indices;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		// position
		vertices.insert(vertices.end(), { mesh->mVertices[i].x, mesh->mVertices[i].y , mesh->mVertices[i].z });

		// tex coords
		if (mesh->mTextureCoords[0]) {
			vertices.insert(vertices.end(), { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y });
		} else {
			vertices.insert(vertices.end(), { 0.0f, 0.0f });
		}

		// normals
		if (mesh->mNormals) {
			vertices.insert(vertices.end(), { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z });
		} else {
			vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
		}

		// tangents
		if (mesh->mTangents) {
			vertices.insert(vertices.end(), { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z });
		} else {
			vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
		}

		// bitangents
		if (mesh->mBitangents) {
			vertices.insert(vertices.end(), { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z });
		} else {
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
	newMesh->Create(&vertices[0], &indices[0], (unsigned int)vertices.size(), (unsigned int)indices.size());
	newMesh->Update(m_Scale);
	m_MeshList.push_back(newMesh);
	m_MeshToTexture.push_back(mesh->mMaterialIndex);
}

/*
 * Loads Materials and corresponding textures - Diffuse (Albedo), Height (Normal)
 */
void Model::LoadMaterials(const aiScene* scene)
{
	m_TextureList.resize(scene->mNumMaterials);
	m_NormalMapList.resize(scene->mNumMaterials);

	for (unsigned int i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];

		m_TextureList[i] = nullptr;

		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			{
				size_t idx = std::string(path.data).rfind("\\");
				std::string filename = std::string(path.data).substr(idx + 1);

				std::string texPath = m_TexturesPath + std::string("/") + filename;

				Log::GetLogger()->info("Texture loaded '{0}'", texPath);

				m_TextureList[i] = TextureLoader::Get()->GetTexture(texPath.c_str(), false, false);

				if (!m_TextureList[i])
				{
					Log::GetLogger()->error("Failed to load texture at '{0}'", texPath);
					// delete m_TextureList[i];
					m_TextureList[i] = nullptr;
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

				Log::GetLogger()->info("Normal Map Texture loaded at '{0}'", texPath);

				m_NormalMapList[i] = TextureLoader::Get()->GetTexture(texPath.c_str(), false, false);

				if (!m_NormalMapList[i])
				{
					Log::GetLogger()->error("Failed to load normal map at '{0}'", texPath);
					// delete m_NormalMapList[i];
					m_NormalMapList[i] = nullptr;
				}
			}
		}

		if (!m_TextureList[i])
		{
			m_TextureList[i] = TextureLoader::Get()->GetTexture("Textures/plain.png", false, false);
		}
	}
}

void Model::Update(glm::vec3 scale)
{
	if (scale != m_Scale)
	{
		for (size_t i = 0; i < m_MeshList.size(); i++)
		{
			m_MeshList[i]->Update(scale);
		}
		m_Scale = scale;
	}
}

/*
 * Render Model by using only Diffuse (Albedo) and Normal textures
 * Texture binding done within the method
 */
void Model::Render(uint32_t txSlotDiffuse, uint32_t txSlotNormal, bool useNormalMaps)
{
	for (size_t i = 0; i < m_MeshList.size(); i++)
	{
		unsigned int materialIndex = m_MeshToTexture[i];

		if (materialIndex < m_TextureList.size() && m_TextureList[materialIndex])
		{
			m_TextureList[materialIndex]->Bind(txSlotDiffuse);
		}

		if (useNormalMaps)
		{
			if (materialIndex < m_NormalMapList.size() && m_NormalMapList[materialIndex])
			{
				m_NormalMapList[materialIndex]->Bind(txSlotNormal);
			}
		}

		m_MeshList[i]->Render();
	}
}

/* 
 * Renders meshes in a model after previously binding all PBR textures:
 * Albedo, Normal, Metalness/Roughness, Emissive, Ambient Occlusion
 */
void Model::RenderPBR()
{
	for (size_t i = 0; i < m_MeshList.size(); i++)
	{
		m_MeshList[i]->Render();
	}
}

void Model::Clear()
{
	for (size_t i = 0; i < m_MeshList.size(); i++)
	{
		if (m_MeshList[i])
		{
			delete m_MeshList[i];
			m_MeshList[i] = nullptr;
		}
	}

	for (size_t i = 0; i < m_TextureList.size(); i++)
	{
		if (m_TextureList[i])
		{
			// delete m_TextureList[i];
			m_TextureList[i] = nullptr;
		}
	}

	m_MeshList.clear();
	m_TextureList.clear();
	m_MeshToTexture.clear();
}

Model::~Model()
{
	Clear();
}
