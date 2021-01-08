#include "HazelMesh.h"

#include <GL/glew.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

#include "../Scene/Entity.h"

#include "../../Math.h"
#include "../../Util.h"
#include "../../ShaderLibrary.h"
#include "../../EnvironmentMap.h"

#include "imgui.h"

#include <filesystem>


namespace Hazel {

#define MESH_DEBUG_LOG 1
#if MESH_DEBUG_LOG
	#define HZ_MESH_LOG(...) MORAVA_CORE_TRACE(__VA_ARGS__)
#else
	#define HZ_MESH_LOG(...)
#endif

	static const uint32_t s_MeshImportFlags =
		aiProcess_CalcTangentSpace |        // Create binormals/tangents just in case
		aiProcess_Triangulate |             // Make sure we're triangles
		aiProcess_SortByPType |             // Split meshes by primitive type
		aiProcess_GenNormals |              // Make sure we have legit normals
		aiProcess_GenUVCoords |             // Convert UVs if required 
		aiProcess_OptimizeMeshes |          // Batch draws where possible
		aiProcess_ValidateDataStructure;    // Validation

	struct LogStream : public Assimp::LogStream
	{
		static void Initialize()
		{
			if (Assimp::DefaultLogger::isNullLogger())
			{
				Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
				Assimp::DefaultLogger::get()->attachStream(new LogStream, Assimp::Logger::Err | Assimp::Logger::Warn);
			}
		}

		virtual void write(const char* message) override
		{
			Log::GetLogger()->error("Assimp error: {0}", message);
		}
	};

	HazelMesh::HazelMesh(const std::string& filename)
	{
		m_FilePath = filename;
		m_BaseTexture = nullptr;
		m_MeshShader = nullptr;
		m_BaseMaterial = nullptr;
		m_IsAnimated = false;

		Create();
	}

	HazelMesh::HazelMesh(const std::string& filename, ::Ref<Shader> shader, Material* material, bool isAnimated)
		: m_MeshShader(shader), m_BaseMaterial(material), m_IsAnimated(isAnimated)
	{
		m_FilePath = filename;

		// TODO: Convert m_BaseMaterial type to Hazel/Renderer/HazelMaterial
		//	if (!m_BaseMaterial) {
		//		SetupDefaultBaseMaterial();
		//	}

		m_BaseTexture = nullptr;

		Create();
	}

	//	HazelMesh::HazelMesh(const std::string& filename)
	//		: m_FilePath(filename)
	//	{
	//		Create();
	//	}

	void HazelMesh::Create()
	{
		LogStream::Initialize();

		Log::GetLogger()->info("Hazel::HazelMesh: Loading mesh: {0}", m_FilePath.c_str());

		m_Importer = std::make_unique<Assimp::Importer>();

		const aiScene* scene = m_Importer->ReadFile(m_FilePath, s_MeshImportFlags);
		if (!scene || !scene->HasMeshes()) {
			Log::GetLogger()->error("Failed to load mesh file: {0}", m_FilePath);
			return;
		}

		m_Scene = scene;

		m_IsAnimated = scene->mAnimations != nullptr;
		// m_MaterialInstance = std::make_shared<MaterialInstance>(m_BaseMaterial);

		// Refactor to HazelRenderer::GetShaderLibrary()->Get()
		if (!m_MeshShader) {
			m_MeshShader = m_IsAnimated ? ShaderLibrary::Get("HazelPBR_Anim") : ShaderLibrary::Get("HazelPBR_Static");
		}
		m_InverseTransform = glm::inverse(Math::Mat4FromAssimpMat4(scene->mRootNode->mTransformation));

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		Log::GetLogger()->info("Hazel::HazelMesh: Master mesh contains {0} submeshes.", scene->mNumMeshes);

		m_Submeshes.reserve(scene->mNumMeshes);
		for (size_t m = 0; m < scene->mNumMeshes; m++)
		{
			aiMesh* mesh = scene->mMeshes[m];

			Submesh& submesh = m_Submeshes.emplace_back();
			submesh.BaseVertex = vertexCount;
			submesh.BaseIndex = indexCount;
			submesh.MaterialIndex = mesh->mMaterialIndex;
			submesh.IndexCount = mesh->mNumFaces * 3;
			submesh.MeshName = mesh->mName.C_Str();
			// m_Submeshes.push_back(submesh);

			vertexCount += mesh->mNumVertices;
			indexCount += submesh.IndexCount;

			HZ_CORE_ASSERT(mesh->HasPositions(), "Meshes require positions.");
			HZ_CORE_ASSERT(mesh->HasNormals(), "Meshes require normals.");

			// Vertices
			if (m_IsAnimated)
			{
				auto& aabb = submesh.BoundingBox;
				aabb.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
				aabb.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

				for (size_t i = 0; i < mesh->mNumVertices; i++)
				{
					AnimatedVertex vertex;
					vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
					vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

					aabb.Min.x = glm::min(vertex.Position.x, aabb.Min.x);
					aabb.Min.y = glm::min(vertex.Position.y, aabb.Min.y);
					aabb.Min.z = glm::min(vertex.Position.z, aabb.Min.z);

					aabb.Max.x = glm::max(vertex.Position.x, aabb.Max.x);
					aabb.Max.y = glm::max(vertex.Position.y, aabb.Max.y);
					aabb.Max.z = glm::max(vertex.Position.z, aabb.Max.z);

					if (mesh->HasTangentsAndBitangents())
					{
						vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
						vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
					}

					if (mesh->HasTextureCoords(0))
						vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

					m_AnimatedVertices.push_back(vertex);
				}
			}
			else
			{
				auto& aabb = submesh.BoundingBox;
				aabb.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
				aabb.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

				for (size_t i = 0; i < mesh->mNumVertices; i++)
				{
					Vertex vertex;
					vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
					vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

					aabb.Min.x = glm::min(vertex.Position.x, aabb.Min.x);
					aabb.Min.y = glm::min(vertex.Position.y, aabb.Min.y);
					aabb.Min.z = glm::min(vertex.Position.z, aabb.Min.z);

					aabb.Max.x = glm::max(vertex.Position.x, aabb.Max.x);
					aabb.Max.y = glm::max(vertex.Position.y, aabb.Max.y);
					aabb.Max.z = glm::max(vertex.Position.z, aabb.Max.z);

					if (mesh->HasTangentsAndBitangents())
					{
						vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
						vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
					}

					if (mesh->HasTextureCoords(0))
						vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

					m_StaticVertices.push_back(vertex);
				}
			}

			// Indices
			for (size_t i = 0; i < mesh->mNumFaces; i++)
			{
				HZ_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices.");
				Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
				m_Indices.push_back(index);

				// Triangle cache
				if (!m_IsAnimated)
				{
					if (index.V1 + submesh.BaseVertex < m_StaticVertices.size() &&
						index.V2 + submesh.BaseVertex < m_StaticVertices.size() &&
						index.V3 + submesh.BaseVertex < m_StaticVertices.size())
					{
						m_TriangleCache[(uint32_t)m].emplace_back(
							m_StaticVertices[index.V1 + submesh.BaseVertex],
							m_StaticVertices[index.V2 + submesh.BaseVertex],
							m_StaticVertices[index.V3 + submesh.BaseVertex]);
					}
				}
			}
		}

		// Display the list of all submeshes
		for (size_t m = 0; m < scene->mNumMeshes; m++) {
			Log::GetLogger()->info("-- Submesh ID {0} NodeName: '{1}'", m, m_Submeshes[m].NodeName);
		}

		TraverseNodes(scene->mRootNode);

		// Bones
		for (size_t m = 0; m < scene->mNumMeshes; m++)
		{
			aiMesh* mesh = scene->mMeshes[m];
			Submesh& submesh = m_Submeshes[m];

			for (size_t i = 0; i < mesh->mNumBones; i++)
			{
				aiBone* bone = mesh->mBones[i];
				std::string boneName(bone->mName.data);
				int boneIndex = 0;

				if (m_BoneMapping.find(boneName) == m_BoneMapping.end())
				{
					// Allocate an index for a new bone
					boneIndex = m_BoneCount;
					m_BoneCount++;
					BoneInfo bi;
					m_BoneInfo.push_back(bi);
					m_BoneInfo[boneIndex].BoneOffset = Math::Mat4FromAssimpMat4(bone->mOffsetMatrix);
					m_BoneMapping[boneName] = boneIndex;
				}
				else
				{
					Log::GetLogger()->info("Found existing bone in map");
					boneIndex = m_BoneMapping[boneName];
				}

				for (size_t j = 0; j < bone->mNumWeights; j++)
				{
					int VertexID = submesh.BaseVertex + bone->mWeights[j].mVertexId;
					float Weight = bone->mWeights[j].mWeight;
					m_AnimatedVertices[VertexID].AddBoneData(boneIndex, Weight);
				}
			}
		}

		// Materials
		if (scene->HasMaterials())
		{
			Log::GetLogger()->info("---- Materials - {0} ----", m_FilePath);

			m_Textures.resize(scene->mNumMaterials);
			m_Materials.resize(scene->mNumMaterials);
			for (uint32_t i = 0; i < scene->mNumMaterials; i++)
			{
				auto aiMaterial = scene->mMaterials[i];
				auto aiMaterialName = aiMaterial->GetName();

				auto mi = new Material(); // m_BaseMaterial
				m_Materials[i] = mi;

				Log::GetLogger()->info("  {0} (Index = {1})", aiMaterialName.data, i);
				aiString aiTexPath;
				uint32_t textureCount = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
				HZ_MESH_LOG("    TextureCount = {0}", textureCount);

				aiColor3D aiColor;
				aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor);

				float shininess, metalness;
				aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
				aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness);

				// float roughness = 1.0f - shininess * 0.01f;
				// roughness *= roughness;
				float roughness = 1.0f - glm::sqrt(shininess / 100.0f);
				HZ_MESH_LOG("    COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);
				HZ_MESH_LOG("    ROUGHNESS = {0}", roughness);
				bool hasAlbedoMap = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
				if (hasAlbedoMap)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::filesystem::path path = m_FilePath;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();
					Log::GetLogger()->info("    Albedo map path = {0}", texturePath);

					Texture* texture = nullptr;
					try {
						texture = new Texture(texturePath.c_str(), false);
					}
					catch (...) {
						Log::GetLogger()->warn("The ALBEDO map failed to load. Loading the default texture placeholder instead.");
						texture = LoadBaseTexture();
					}

					if (texture->IsLoaded())
					{
						m_Textures[i] = texture;
						m_MeshShader->setInt("u_AlbedoTexture", m_Textures[i]->GetID());
						m_MeshShader->setFloat("u_AlbedoTexToggle", 1.0f);
					}
					else
					{
						Log::GetLogger()->error("Could not load texture: {0}", texturePath);
						// Fallback to albedo color
						m_MeshShader->setVec3("u_AlbedoColor", glm::vec3{ aiColor.r, aiColor.g, aiColor.b });
						HZ_MESH_LOG("Mesh has no Albedo map.");
					}
				}
				else
				{
					m_MeshShader->setVec3("u_AlbedoColor", glm::vec3{ aiColor.r, aiColor.g, aiColor.b });
					Log::GetLogger()->info("    No albedo map");
				}

				// Normal maps
				m_MeshShader->setFloat("u_NormalTexToggle", 0.0f);
				if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::filesystem::path path = m_FilePath;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();
					HZ_MESH_LOG("    Normal map path = {0}", texturePath);

					Texture* texture = nullptr;
					try {
						texture = new Texture(texturePath.c_str(), false);
					}
					catch (...) {
						Log::GetLogger()->warn("The NORMAL map failed to load. Loading the default texture placeholder instead.");
						texture = new Texture("Textures/normal_map_default.png");
					}

					if (texture->IsLoaded())
					{
						m_MeshShader->setInt("u_NormalTexture", texture->GetID());
						m_MeshShader->setFloat("u_NormalTexToggle", 1.0f);
					}
					else
					{
						Log::GetLogger()->error("    Could not load texture: {0}", texturePath);
					}
				}
				else
				{
					Log::GetLogger()->info("    No normal map");
				}

				// Roughness map
				// m_MeshShader->setFloat("u_Roughness", 1.0f);
				// m_MeshShader->setFloat("u_RoughnessTexToggle", 0.0f);
				if (aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::filesystem::path path = m_FilePath;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();
					// HZ_MESH_LOG("  Roughness map path = '{0}'", texturePath);

					Texture* texture = nullptr;
					try {
						texture = new Texture(texturePath.c_str(), false);
					}
					catch (...) {
						Log::GetLogger()->warn("The ROUGHNESS map failed to load. Loading the default texture placeholder instead.");
						texture = LoadBaseTexture();
					}

					if (texture->IsLoaded())
					{
						HZ_MESH_LOG("  Roughness map path = '{0}'", texturePath);
						m_MeshShader->setInt("u_RoughnessTexture", texture->GetID());
						m_MeshShader->setFloat("u_RoughnessTexToggle", 1.0f);
					}
					else
					{
						Log::GetLogger()->error("    Could not load texture: {0}", texturePath);
					}
				}
				else
				{
					Log::GetLogger()->info("    No roughness map");
					m_MeshShader->setFloat("u_Roughness", roughness);
				}

#if 0
				// Metalness map (or is it??)
				if (aiMaterial->Get("$raw.ReflectionFactor|file", aiPTI_String, 0, aiTexPath) == AI_SUCCESS)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::filesystem::path path = m_FilePath;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();

					Texture* texture = nullptr;
					try {
						texture = new Texture(texturePath.c_str(), false);
					}
					catch (...) {
						Log::GetLogger()->warn("The METALNESS map failed to load. Loading the default texture placeholder instead.");
						texture = LoadBaseTexture();
					}

					if (texture->IsLoaded())
					{
						HZ_MESH_LOG("    Metalness map path = {0}", texturePath);
						m_MeshShader->setInt("u_MetalnessTexture", texture->GetID());
						m_MeshShader->setFloat("u_MetalnessTexToggle", 1.0f);
					}
					else
					{
						Log::GetLogger()->error("Could not load texture: {0}", texturePath);
					}
				}
				else
				{
					Log::GetLogger()->info("    No metalness texture");
					m_MeshShader->setFloat("u_Metalness", metalness);
				}
#endif

				bool metalnessTextureFound = false;
				for (uint32_t i = 0; i < aiMaterial->mNumProperties; i++)
				{
					auto prop = aiMaterial->mProperties[i];

#if DEBUG_PRINT_ALL_PROPS
					HZ_MESH_LOG("Material Property:");
					HZ_MESH_LOG("  Name = {0}", prop->mKey.data);
					// HZ_MESH_LOG("  Type = {0}", prop->mType);
					// HZ_MESH_LOG("  Size = {0}", prop->mDataLength);
					float data = *(float*)prop->mData;
					HZ_MESH_LOG("  Value = {0}", data);

					switch (prop->mSemantic)
					{
					case aiTextureType_NONE:
						HZ_MESH_LOG("  Semantic = aiTextureType_NONE");
						break;
					case aiTextureType_DIFFUSE:
						HZ_MESH_LOG("  Semantic = aiTextureType_DIFFUSE");
						break;
					case aiTextureType_SPECULAR:
						HZ_MESH_LOG("  Semantic = aiTextureType_SPECULAR");
						break;
					case aiTextureType_AMBIENT:
						HZ_MESH_LOG("  Semantic = aiTextureType_AMBIENT");
						break;
					case aiTextureType_EMISSIVE:
						HZ_MESH_LOG("  Semantic = aiTextureType_EMISSIVE");
						break;
					case aiTextureType_HEIGHT:
						HZ_MESH_LOG("  Semantic = aiTextureType_HEIGHT");
						break;
					case aiTextureType_NORMALS:
						HZ_MESH_LOG("  Semantic = aiTextureType_NORMALS");
						break;
					case aiTextureType_SHININESS:
						HZ_MESH_LOG("  Semantic = aiTextureType_SHININESS");
						break;
					case aiTextureType_OPACITY:
						HZ_MESH_LOG("  Semantic = aiTextureType_OPACITY");
						break;
					case aiTextureType_DISPLACEMENT:
						HZ_MESH_LOG("  Semantic = aiTextureType_DISPLACEMENT");
						break;
					case aiTextureType_LIGHTMAP:
						HZ_MESH_LOG("  Semantic = aiTextureType_LIGHTMAP");
						break;
					case aiTextureType_REFLECTION:
						HZ_MESH_LOG("  Semantic = aiTextureType_REFLECTION");
						break;
					case aiTextureType_UNKNOWN:
						HZ_MESH_LOG("  Semantic = aiTextureType_UNKNOWN");
						break;
					}
#endif

					if (prop->mType == aiPTI_String)
					{
						uint32_t strLength = *(uint32_t*)prop->mData;
						std::string str(prop->mData + 4, strLength);

						std::string key = prop->mKey.data;
						if (key == "$raw.ReflectionFactor|file")
						{
							metalnessTextureFound = true;

							// TODO: Temp - this should be handled by Hazel's filesystem
							std::filesystem::path path = m_FilePath;
							auto parentPath = path.parent_path();
							parentPath /= str;
							std::string texturePath = parentPath.string();
							HZ_MESH_LOG("    Metalness map path = {0}", texturePath);

							Texture* texture = nullptr;
							try {
								texture = new Texture(texturePath.c_str(), false);
							}
							catch (...) {
								Log::GetLogger()->warn("The METALNESS map failed to load. Loading the default texture placeholder instead.");
								texture = LoadBaseTexture();
							}

							if (texture->IsLoaded())
							{
								m_MeshShader->setInt("u_MetalnessTexture", texture->GetID());
								m_MeshShader->setFloat("u_MetalnessTexToggle", 1.0f);
							}
							else
							{
								Log::GetLogger()->error("    Could not load texture: {0}", texturePath);
								m_MeshShader->setFloat("u_Metalness", metalness);
								m_MeshShader->setFloat("u_MetalnessTexToggle", 0.0f);
							}
							break;
						}
					}
				}

				if (!metalnessTextureFound)
				{
					Log::GetLogger()->info("    No metalness map");

					m_MeshShader->setFloat("u_Metalness", metalness);
					m_MeshShader->setFloat("u_MetalnessTexToggle", 0.0f);
				}
			}
			HZ_MESH_LOG("------------------------");
		}

		Log::GetLogger()->info("Hazel::HazelMesh: Creating a Vertex Array...");

		m_VertexArray = VertexArray::Create();
		if (m_IsAnimated)
		{
			Log::GetLogger()->info("Hazel::HazelMesh: Creating a Vertex Buffer...");
			auto vb = VertexBuffer::Create(m_AnimatedVertices.data(), (uint32_t)m_AnimatedVertices.size() * sizeof(AnimatedVertex));
			vb->SetLayout({
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Int4,   "a_BoneIDs" },
				{ ShaderDataType::Float4, "a_BoneWeights" },
				});
			m_VertexArray->AddVertexBuffer(vb);
		}
		else
		{
			Log::GetLogger()->info("Hazel::HazelMesh: Creating a Vertex Buffer...");
			auto vb = VertexBuffer::Create(m_StaticVertices.data(), (uint32_t)m_StaticVertices.size() * sizeof(Vertex));
			vb->SetLayout({
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				//	{ ShaderDataType::Int4,   "a_BoneIDs" },
				//	{ ShaderDataType::Float4, "a_BoneWeights" },
				});
			m_VertexArray->AddVertexBuffer(vb);
		}

		Log::GetLogger()->info("Hazel::HazelMesh: Creating an Index Buffer...");
		auto ib = IndexBuffer::Create(m_Indices.data(), (uint32_t)m_Indices.size() * sizeof(Index));
		m_VertexArray->SetIndexBuffer(ib);

		Log::GetLogger()->info("Hazel::HazelMesh: Total vertices: {0}", m_IsAnimated ? m_StaticVertices.size() : m_AnimatedVertices.size());
		Log::GetLogger()->info("Hazel::HazelMesh: Total indices: {0}", m_Indices.size());
	}

	HazelMesh::~HazelMesh()
	{
		for (auto material : m_Materials) {
			delete material;
		}

		for (auto texture : m_Textures) {
			if (texture != nullptr) {
				delete texture;
			}
		}

		//	for (Submesh* submesh : m_Submeshes)
		//		delete submesh;

		delete m_IndexBuffer;
	}

	void HazelMesh::OnUpdate(float ts, bool debug)
	{
		m_WorldTime += ts;

		if (m_IsAnimated && m_Scene->mAnimations && m_AnimationPlaying)
		{

			float ticksPerSecond = (float)(m_Scene->mAnimations[0]->mTicksPerSecond != 0 ? m_Scene->mAnimations[0]->mTicksPerSecond : 25.0f) * m_TimeMultiplier;
			m_AnimationTime += ts * ticksPerSecond;
			m_AnimationTime = fmod(m_AnimationTime, (float)m_Scene->mAnimations[0]->mDuration);

			//	if (debug) {
			//		Log::GetLogger()->info("HazelMesh::OnUpdate ts: {0} m_AnimationTime: {1} mDuration {2} ticksPerSecond {3}",
			//			ts, m_AnimationTime, m_Scene->mAnimations[0]->mDuration, ticksPerSecond);
			//	}
		}

		// TODO: We only need to recalc bones if rendering has been requested at the current animation frame
		if (m_IsAnimated) {
			BoneTransform(m_AnimationTime);
		}
	}

	static std::string LevelToSpaces(uint32_t level)
	{
		std::string result = "";
		for (uint32_t i = 0; i < level; i++)
			result += "--";
		return result;
	}

	void HazelMesh::TraverseNodes(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 transform = parentTransform * Math::Mat4FromAssimpMat4(node->mTransformation);
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			uint32_t mesh = node->mMeshes[i];
			Submesh& submesh = m_Submeshes[mesh];
			submesh.NodeName = node->mName.C_Str();
			submesh.MeshName = m_Scene->mMeshes[mesh]->mName.C_Str();
			submesh.Transform = transform;
		}

		HZ_MESH_LOG("{0} {1}", LevelToSpaces(level), node->mName.C_Str());

		for (uint32_t i = 0; i < node->mNumChildren; i++)
			TraverseNodes(node->mChildren[i], transform, level + 1);
	}

	uint32_t HazelMesh::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
		{
			if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
				return i;
		}

		return 0;
	}

	uint32_t HazelMesh::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		if (pNodeAnim->mNumRotationKeys <= 0)
			Log::GetLogger()->error("pNodeAnim->mNumRotationKeys <= 0");

		for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
		{
			if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
				return i;
		}

		return 0;
	}


	uint32_t HazelMesh::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		if (pNodeAnim->mNumScalingKeys <= 0)
			Log::GetLogger()->error("pNodeAnim->mNumScalingKeys <= 0");

		for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
		{
			if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
				return i;
		}

		return 0;
	}


	glm::vec3 HazelMesh::InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim)
	{
		if (nodeAnim->mNumPositionKeys == 1)
		{
			// No interpolation necessary for single value
			auto v = nodeAnim->mPositionKeys[0].mValue;
			return { v.x, v.y, v.z };
		}

		uint32_t PositionIndex = FindPosition(animationTime, nodeAnim);
		uint32_t NextPositionIndex = (PositionIndex + 1);

		if (NextPositionIndex >= nodeAnim->mNumPositionKeys)
			Log::GetLogger()->error("NextPositionIndex >= nodeAnim->mNumPositionKeys");

		float DeltaTime = (float)(nodeAnim->mPositionKeys[NextPositionIndex].mTime - nodeAnim->mPositionKeys[PositionIndex].mTime);
		float Factor = (animationTime - (float)nodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
		if (Factor < 0.0f)
			Factor = 0.0f;

		if (Factor > 1.0f)
			Log::GetLogger()->error("Factor must be below 1.0f");

		const aiVector3D& Start = nodeAnim->mPositionKeys[PositionIndex].mValue;
		const aiVector3D& End = nodeAnim->mPositionKeys[NextPositionIndex].mValue;
		aiVector3D Delta = End - Start;
		auto aiVec = Start + Factor * Delta;
		return { aiVec.x, aiVec.y, aiVec.z };
	}


	glm::quat HazelMesh::InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim)
	{
		if (nodeAnim->mNumRotationKeys == 1)
		{
			// No interpolation necessary for single value
			auto v = nodeAnim->mRotationKeys[0].mValue;
			return glm::quat(v.w, v.x, v.y, v.z);
		}

		uint32_t RotationIndex = FindRotation(animationTime, nodeAnim);
		uint32_t NextRotationIndex = (RotationIndex + 1);

		if (NextRotationIndex >= nodeAnim->mNumRotationKeys)
			Log::GetLogger()->error("NextRotationIndex >= nodeAnim->mNumRotationKeys");


		float DeltaTime = (float)(nodeAnim->mRotationKeys[NextRotationIndex].mTime - nodeAnim->mRotationKeys[RotationIndex].mTime);
		float Factor = (animationTime - (float)nodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
		if (Factor < 0.0f)
			Factor = 0.0f;

		if (Factor > 1.0f)
			Log::GetLogger()->error("Factor must be below 1.0f");

		const aiQuaternion& StartRotationQ = nodeAnim->mRotationKeys[RotationIndex].mValue;
		const aiQuaternion& EndRotationQ = nodeAnim->mRotationKeys[NextRotationIndex].mValue;
		auto q = aiQuaternion();
		aiQuaternion::Interpolate(q, StartRotationQ, EndRotationQ, Factor);
		q = q.Normalize();
		return glm::quat(q.w, q.x, q.y, q.z);
	}


	glm::vec3 HazelMesh::InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim)
	{
		if (nodeAnim->mNumScalingKeys == 1)
		{
			// No interpolation necessary for single value
			auto v = nodeAnim->mScalingKeys[0].mValue;
			return { v.x, v.y, v.z };
		}

		uint32_t index = FindScaling(animationTime, nodeAnim);
		uint32_t nextIndex = (index + 1);

		if (nextIndex >= nodeAnim->mNumScalingKeys)
			Log::GetLogger()->error("nextIndex >= nodeAnim->mNumScalingKeys");

		float deltaTime = (float)(nodeAnim->mScalingKeys[nextIndex].mTime - nodeAnim->mScalingKeys[index].mTime);
		float factor = (animationTime - (float)nodeAnim->mScalingKeys[index].mTime) / deltaTime;
		if (factor < 0.0f)
			factor = 0.0f;

		if (factor > 1.0f)
			Log::GetLogger()->error("Factor must be below 1.0f");

		const auto& start = nodeAnim->mScalingKeys[index].mValue;
		const auto& end = nodeAnim->mScalingKeys[nextIndex].mValue;
		auto delta = end - start;
		auto aiVec = start + factor * delta;
		return { aiVec.x, aiVec.y, aiVec.z };
	}

	void HazelMesh::SetupDefaultBaseMaterial()
	{
		// Setup default Material
		TextureInfo textureInfoDefault = {};
		textureInfoDefault.albedo    = "Textures/plain.png";
		textureInfoDefault.normal    = "Textures/normal_map_default.png";
		textureInfoDefault.metallic  = "Textures/plain.png";
		textureInfoDefault.roughness = "Textures/plain.png";
		textureInfoDefault.emissive  = "Texture/plain.png";
		textureInfoDefault.ao        = "Textures/plain.png";
		m_BaseMaterial = new Material(textureInfoDefault, 0.0f, 0.0f);
	}

	Texture* HazelMesh::LoadBaseTexture()
	{
		if (!m_BaseTexture) {
			try {
				m_BaseTexture = new Texture("Textures/plain.png");
			}
			catch (...) {
				Log::GetLogger()->warn("Failed to load the base texture!");
				m_BaseTexture = nullptr;
			}
		}

		return m_BaseTexture;
	}

	void HazelMesh::ImGuiNodeHierarchy(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 localTransform = Math::Mat4FromAssimpMat4(node->mTransformation);
		glm::mat4 transform = parentTransform * localTransform;

		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			uint32_t mesh = node->mMeshes[i];
			m_Submeshes[mesh].NodeName = node->mName.C_Str();
			m_Submeshes[mesh].Transform = transform;
		}

		if (ImGui::TreeNode(node->mName.C_Str()))
		{
			auto [translation, rotation, scale] = Math::GetTransformDecomposition(transform);
			glm::vec3 rotationVec3 = glm::degrees(glm::eulerAngles(rotation));
			ImGui::Text("World Transform");
			ImGui::Text("  Translation: %.2f %.2f %.2f", translation.x, translation.y, translation.z);
			ImGui::Text("  Rotation:    %.2f %.2f %.2f", rotationVec3.x, rotationVec3.y, rotationVec3.z);
			ImGui::Text("  Scale:       %.2f %.2f %.2f", scale.x, scale.y, scale.z);

			/****
			{
				auto [translation, rotation, scale] = Math::GetTransformDecomposition(localTransform);
				glm::vec3 rotationVec3 = glm::degrees(glm::eulerAngles(rotation));
				ImGui::Text("Local Transform");
				ImGui::Text("  Translation: %.2f %.2f %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Rotation:    %.2f %.2f %.2f", rotationVec3.x, rotationVec3.y, rotationVec3.z);
				ImGui::Text("  Scale:       %.2f %.2f %.2f", scale.x, scale.y, scale.z);
			}
			****/

			for (uint32_t i = 0; i < node->mNumChildren; i++)
				ImGuiNodeHierarchy(node->mChildren[i], transform, level + 1);

			ImGui::TreePop();
		}
	}

	void HazelMesh::OnImGuiRender(uint32_t id)
	{
		if (!m_Scene) {
			Log::GetLogger()->error("Mesh: Scene not initialized!");
			return;
		}

		// Mesh Hierarchy
		ImGui::Begin("Mesh Hierarchy");
		ImGuiNodeHierarchy(m_Scene->mRootNode, glm::mat4(1.0f), 0);
		ImGui::End();

		ImGui::Begin("Mesh Debug");
		std::string meshFileName = std::to_string(id) + ". " + Util::GetFileNameFromFullPath(m_FilePath);
		if (ImGui::CollapsingHeader(meshFileName.c_str()))
		{
			if (m_IsAnimated && m_Scene->mAnimations)
			{
				ImGui::Indent(10.0f);
				if (ImGui::CollapsingHeader("Animation"))
				{
					if (ImGui::Button(m_AnimationPlaying ? "Pause" : "Play"))
						m_AnimationPlaying = !m_AnimationPlaying;

					ImGui::SliderFloat("##AnimationTime", &m_AnimationTime, 0.0f, (float)m_Scene->mAnimations[0]->mDuration);
					ImGui::DragFloat("Time Scale", &m_TimeMultiplier, 0.05f, 0.0f, 100.0f);
				}
				ImGui::Unindent(10.0f);
			}
		}
		ImGui::End();
	}

	void HazelMesh::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& parentTransform)
	{
		std::string name(pNode->mName.data);
		glm::mat4 nodeTransform(Math::Mat4FromAssimpMat4(pNode->mTransformation));

		aiAnimation* animation;
		aiNodeAnim* nodeAnim;

		if (m_IsAnimated && m_Scene->mAnimations)
		{
			animation = m_Scene->mAnimations[0];
			nodeAnim = FindNodeAnim(animation, name);

			if (nodeAnim)
			{
				glm::vec3 translation = InterpolateTranslation(AnimationTime, nodeAnim);
				glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, translation.z));

				glm::quat rotation = InterpolateRotation(AnimationTime, nodeAnim);
				glm::mat4 rotationMatrix = glm::toMat4(rotation);

				glm::vec3 scale = InterpolateScale(AnimationTime, nodeAnim);
				glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, scale.z));

				nodeTransform = translationMatrix * rotationMatrix * scaleMatrix;
			}
		}

		glm::mat4 transform = parentTransform * nodeTransform;

		if (m_BoneMapping.find(name) != m_BoneMapping.end())
		{
			uint32_t BoneIndex = m_BoneMapping[name];
			m_BoneInfo[BoneIndex].FinalTransformation = m_InverseTransform * transform * m_BoneInfo[BoneIndex].BoneOffset;
		}

		for (uint32_t i = 0; i < pNode->mNumChildren; i++)
			ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], transform);
	}

	aiNodeAnim* HazelMesh::FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
	{
		for (uint32_t i = 0; i < animation->mNumChannels; i++)
		{
			aiNodeAnim* nodeAnim = animation->mChannels[i];
			if (std::string(nodeAnim->mNodeName.data) == nodeName)
				return nodeAnim;
		}
		return nullptr;
	}

	std::string HazelMesh::GetSubmeshMaterialName(Ref<HazelMesh> mesh, Hazel::Submesh& submesh, Entity entity, const std::map<std::string, std::string>& submeshMaterials)
	{
		std::string materialName = "";

		if (submeshMaterials.contains(submesh.MeshName)) {
			materialName = submeshMaterials.at(submesh.MeshName);
		}
		else if (entity && entity.HasComponent<Hazel::MaterialComponent>()) {
			materialName = entity.GetComponent<Hazel::MaterialComponent>().Material->GetName();
		}
		else {
			std::string meshName = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(mesh->GetFilePath()));
			materialName = meshName + "_" + std::to_string(submesh.MaterialIndex);
		}

		return materialName;
	}

	void HazelMesh::DeleteSubmesh(Submesh submesh)
	{
		for (auto& iterator = m_Submeshes.cbegin(); iterator != m_Submeshes.cend();)
		{
			if (iterator->MeshName == submesh.MeshName) {
				iterator = m_Submeshes.erase(iterator++);
				Log::GetLogger()->debug("HazelMesh::DeleteSubmesh erase '{0}'", submesh.MeshName);
			}
			else {
				++iterator;
			}
		}
	}

	void HazelMesh::CloneSubmesh(Submesh submesh)
	{
		EntitySelection::s_SelectionContext.clear();

		Submesh* submeshCopy = new Submesh(submesh);
		std::string appendix = Util::randomString(2);
		submeshCopy->MeshName += "." + appendix;
		submeshCopy->NodeName += "." + appendix;
		m_Submeshes.push_back(*submeshCopy);
	}

	void HazelMesh::BoneTransform(float time)
	{
		ReadNodeHierarchy(time, m_Scene->mRootNode, glm::mat4(1.0f));
		m_BoneTransforms.resize(m_BoneCount);
		for (size_t i = 0; i < m_BoneCount; i++)
			m_BoneTransforms[i] = m_BoneInfo[i].FinalTransformation;
	}

	void HazelMesh::DumpVertexBuffer()
	{
		// TODO: Convert to ImGui
		HZ_MESH_LOG("------------------------------------------------------");
		HZ_MESH_LOG("Vertex Buffer Dump");
		HZ_MESH_LOG("Mesh: {0}", m_FilePath);
		if (m_IsAnimated)
		{
			for (size_t i = 0; i < m_AnimatedVertices.size(); i++)
			{
				auto& vertex = m_AnimatedVertices[i];
				HZ_MESH_LOG("Vertex:   {0}", i);
				HZ_MESH_LOG("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
				HZ_MESH_LOG("Normal:   {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
				HZ_MESH_LOG("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
				HZ_MESH_LOG("Tangent:  {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
				HZ_MESH_LOG("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
				HZ_MESH_LOG("--");
			}
		}
		else
		{
			for (size_t i = 0; i < m_StaticVertices.size(); i++)
			{
				auto& vertex = m_StaticVertices[i];
				HZ_MESH_LOG("Vertex:   {0}", i);
				HZ_MESH_LOG("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
				HZ_MESH_LOG("Normal:   {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
				HZ_MESH_LOG("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
				HZ_MESH_LOG("Tangent:  {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
				HZ_MESH_LOG("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
				HZ_MESH_LOG("--");
			}
		}
		HZ_MESH_LOG("------------------------------------------------------");
	}

	const std::vector<Triangle> HazelMesh::GetTriangleCache(uint32_t index) const
	{
		if (index < m_TriangleCache.size())
		{
			return m_TriangleCache.at(index);
		}
		return std::vector<Triangle>();
	}

	void HazelMesh::Render(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, EnvMapMaterial*>& envMapMaterials)
	{
		EnvMapMaterial* envMapMaterial = nullptr;

		m_VertexArray->Bind();

		for (Submesh& submesh : m_Submeshes)
		{
			m_MeshShader->Bind();

			for (size_t i = 0; i < m_BoneTransforms.size(); i++)
			{
				std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
				m_MeshShader->setMat4(uniformName, m_BoneTransforms[i]);
			}

			m_MeshShader->setMat4("u_Transform", transform * submesh.Transform);

			// Manage materials (PBR texture binding)
			if (m_BaseMaterial) {
				m_BaseMaterial->GetTextureAlbedo()->Bind(samplerSlot + 0);
				m_BaseMaterial->GetTextureNormal()->Bind(samplerSlot + 1);
				m_BaseMaterial->GetTextureMetallic()->Bind(samplerSlot + 2);
				m_BaseMaterial->GetTextureRoughness()->Bind(samplerSlot + 3);
				m_BaseMaterial->GetTextureEmissive()->Bind(samplerSlot + 4);
				m_BaseMaterial->GetTextureAO()->Bind(samplerSlot + 5);
			}

			std::string materialName = Hazel::HazelMesh::GetSubmeshMaterialName(this, submesh, Entity{}, std::map<std::string, std::string>());

			if (envMapMaterials.contains(materialName))
			{
				envMapMaterial = envMapMaterials.at(materialName);
				envMapMaterial->GetAlbedoInput().TextureMap->Bind(samplerSlot + 0);
				envMapMaterial->GetNormalInput().TextureMap->Bind(samplerSlot + 1);
				envMapMaterial->GetMetalnessInput().TextureMap->Bind(samplerSlot + 2);
				envMapMaterial->GetRoughnessInput().TextureMap->Bind(samplerSlot + 3);
				envMapMaterial->GetEmissiveInput().TextureMap->Bind(samplerSlot + 4);
				envMapMaterial->GetAOInput().TextureMap->Bind(samplerSlot + 5);
			}

			auto material = m_Materials[submesh.MaterialIndex];
			if (material->GetFlag(MaterialFlag::DepthTest)) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}

			glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t)* submesh.BaseIndex), submesh.BaseVertex);
		}
	}

	void HazelMesh::RenderSubmeshes(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, EnvMapMaterial*>& envMapMaterials, Entity entity)
	{
		for (Hazel::Submesh submesh : m_Submeshes)
		{
			submesh.Render(this, m_MeshShader, transform, samplerSlot, envMapMaterials, entity);
		}
	}

	void Submesh::Render(HazelMesh* parentMesh, ::Ref<Shader> shader, glm::mat4 transform, uint32_t samplerSlot,
		const std::map<std::string, EnvMapMaterial*>& envMapMaterials, Entity entity)
	{
		glm::mat4 submeshTransform;
		if (entity && entity.HasComponent<TransformComponent>()) {
			submeshTransform = entity.GetComponent<TransformComponent>().GetTransform();
		}
		else {
			submeshTransform = transform;
		}

		EnvMapMaterial* envMapMaterial = nullptr;

		parentMesh->GetVertexArray().Raw()->Bind();

		shader->Bind();

		for (size_t i = 0; i < parentMesh->GetBoneTransforms().size(); i++)
		{
			std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
			shader->setMat4(uniformName, parentMesh->GetBoneTransforms()[i]);
		}

		shader->setMat4("u_Transform", submeshTransform * Transform);

		// Manage materials (PBR texture binding)
		if (m_BaseMaterial) {
			m_BaseMaterial->GetTextureAlbedo()->Bind(samplerSlot + 0);
			m_BaseMaterial->GetTextureNormal()->Bind(samplerSlot + 1);
			m_BaseMaterial->GetTextureMetallic()->Bind(samplerSlot + 2);
			m_BaseMaterial->GetTextureRoughness()->Bind(samplerSlot + 3);
			m_BaseMaterial->GetTextureEmissive()->Bind(samplerSlot + 4);
			m_BaseMaterial->GetTextureAO()->Bind(samplerSlot + 5);
		}

		std::string materialName = Hazel::HazelMesh::GetSubmeshMaterialName(parentMesh, *this, entity, EnvironmentMap::s_SubmeshMaterials);

		if (envMapMaterials.contains(materialName))
		{
			envMapMaterial = envMapMaterials.at(materialName);
			envMapMaterial->GetAlbedoInput().TextureMap->Bind(samplerSlot + 0);
			envMapMaterial->GetNormalInput().TextureMap->Bind(samplerSlot + 1);
			envMapMaterial->GetMetalnessInput().TextureMap->Bind(samplerSlot + 2);
			envMapMaterial->GetRoughnessInput().TextureMap->Bind(samplerSlot + 3);
			envMapMaterial->GetEmissiveInput().TextureMap->Bind(samplerSlot + 4);
			envMapMaterial->GetAOInput().TextureMap->Bind(samplerSlot + 5);
		}

		auto material = parentMesh->GetMaterials()[MaterialIndex];
		if (material->GetFlag(MaterialFlag::DepthTest)) {
			glEnable(GL_DEPTH_TEST);
		}
		else {
			glDisable(GL_DEPTH_TEST);
		}

		glDrawElementsBaseVertex(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * BaseIndex), BaseVertex);
	}
}
