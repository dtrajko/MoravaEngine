#include "MeshAnimPBR.h"

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

#include "../../Math.h"

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

	MeshAnimPBR::MeshAnimPBR(const std::string& filename, Shader* shader, Material* material, bool isAnimated)
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

	//	MeshAnimPBR::MeshAnimPBR(const std::string& filename)
	//		: m_FilePath(filename)
	//	{
	//		Create();
	//	}

	void MeshAnimPBR::Create()
	{
		LogStream::Initialize();

		Log::GetLogger()->info("Hazel::MeshAnimPBR: Loading mesh: {0}", m_FilePath.c_str());

		m_Importer = std::make_unique<Assimp::Importer>();

		const aiScene* scene = m_Importer->ReadFile(m_FilePath, s_MeshImportFlags);
		if (!scene || !scene->HasMeshes())
			Log::GetLogger()->error("Failed to load mesh file: {0}", m_FilePath);

		m_Scene = scene;

		m_IsAnimated = scene->mAnimations != nullptr;
		// m_MaterialInstance = std::make_shared<MaterialInstance>(m_BaseMaterial);
		m_InverseTransform = glm::inverse(Math::Mat4FromAssimpMat4(scene->mRootNode->mTransformation));

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		m_Submeshes.reserve(scene->mNumMeshes);

		Log::GetLogger()->info("Hazel::MeshAnimPBR: Master mesh contains {0} submeshes.", scene->mNumMeshes);

		for (size_t m = 0; m < scene->mNumMeshes; m++)
		{
			aiMesh* mesh = scene->mMeshes[m];

			Submesh* submesh = new Submesh();
			submesh->BaseVertex = vertexCount;
			submesh->BaseIndex = indexCount;
			submesh->MaterialIndex = mesh->mMaterialIndex;
			submesh->IndexCount = mesh->mNumFaces * 3;
			submesh->MeshName = mesh->mName.C_Str();
			m_Submeshes.push_back(submesh);

			vertexCount += mesh->mNumVertices;
			indexCount += submesh->IndexCount;

			if (!mesh->HasPositions())
				Log::GetLogger()->error("Meshes require positions.");

			if (!mesh->HasNormals())
				Log::GetLogger()->error("Meshes require normals.");

			// Vertices
			submesh->Min = glm::vec3(FLT_MAX);
			submesh->Max = glm::vec3(FLT_MIN);

			for (size_t i = 0; i < mesh->mNumVertices; i++)
			{
				AnimatedVertex vertex;
				vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
				vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

				submesh->Min.x = glm::min(vertex.Position.x, submesh->Min.x);
				submesh->Min.y = glm::min(vertex.Position.y, submesh->Min.y);
				submesh->Min.z = glm::min(vertex.Position.z, submesh->Min.z);

				submesh->Max.x = glm::max(vertex.Position.x, submesh->Max.x);
				submesh->Max.y = glm::max(vertex.Position.y, submesh->Max.y);
				submesh->Max.z = glm::max(vertex.Position.z, submesh->Max.z);

				if (mesh->HasTangentsAndBitangents())
				{
					vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
					vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
				}

				if (mesh->HasTextureCoords(0))
					vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

				m_AnimatedVertices.push_back(vertex);
			}

			// Indices
			for (size_t i = 0; i < mesh->mNumFaces; i++)
			{
				if (mesh->mFaces[i].mNumIndices != 3)
					Log::GetLogger()->error("Must have 3 indices.");

				Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
				m_Indices.push_back(index);
			}
		}

		// Display the list of all submeshes
		for (size_t m = 0; m < scene->mNumMeshes; m++) {
			Log::GetLogger()->info("-- Submesh ID {0} NodeName: '{1}'", m, m_Submeshes[m]->NodeName);
		}

		TraverseNodes(scene->mRootNode);

		// Bones
		for (size_t m = 0; m < scene->mNumMeshes; m++)
		{
			aiMesh* mesh = scene->mMeshes[m];
			Submesh* submesh = m_Submeshes[m];

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
					int VertexID = submesh->BaseVertex + bone->mWeights[j].mVertexId;
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
				Log::GetLogger()->info("    TextureCount = {0}", textureCount);

				aiColor3D aiColor;
				aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor);

				float shininess, metalness;
				aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
				aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness);

				// float roughness = 1.0f - shininess * 0.01f;
				// roughness *= roughness;
				float roughness = 1.0f - glm::sqrt(shininess / 100.0f);
				Log::GetLogger()->info("    COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);
				Log::GetLogger()->info("    ROUGHNESS = {0}", roughness);
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
					Log::GetLogger()->info("    Normal map path = {0}", texturePath);

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
					// Log::GetLogger()->info("  Roughness map path = '{0}'", texturePath);

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
					std::filesystem::path path = filename;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();

					auto texture = Texture2D::Create(texturePath);
					if (texture->Loaded())
					{
						Log::GetLogger()->info("    Metalness map path = {0}", texturePath);
						mi->Set("u_MetalnessTexture", texture);
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
					mi->Set("u_Metalness", metalness);
				}
#endif

				bool metalnessTextureFound = false;
				for (uint32_t i = 0; i < aiMaterial->mNumProperties; i++)
				{
					auto prop = aiMaterial->mProperties[i];

#if DEBUG_PRINT_ALL_PROPS
					Log::GetLogger()->info("Material Property:");
					Log::GetLogger()->info("  Name = {0}", prop->mKey.data);
					// Log::GetLogger()->info("  Type = {0}", prop->mType);
					// Log::GetLogger()->info("  Size = {0}", prop->mDataLength);
					float data = *(float*)prop->mData;
					Log::GetLogger()->info("  Value = {0}", data);

					switch (prop->mSemantic)
					{
					case aiTextureType_NONE:
						Log::GetLogger()->info("  Semantic = aiTextureType_NONE");
						break;
					case aiTextureType_DIFFUSE:
						Log::GetLogger()->info("  Semantic = aiTextureType_DIFFUSE");
						break;
					case aiTextureType_SPECULAR:
						Log::GetLogger()->info("  Semantic = aiTextureType_SPECULAR");
						break;
					case aiTextureType_AMBIENT:
						Log::GetLogger()->info("  Semantic = aiTextureType_AMBIENT");
						break;
					case aiTextureType_EMISSIVE:
						Log::GetLogger()->info("  Semantic = aiTextureType_EMISSIVE");
						break;
					case aiTextureType_HEIGHT:
						Log::GetLogger()->info("  Semantic = aiTextureType_HEIGHT");
						break;
					case aiTextureType_NORMALS:
						Log::GetLogger()->info("  Semantic = aiTextureType_NORMALS");
						break;
					case aiTextureType_SHININESS:
						Log::GetLogger()->info("  Semantic = aiTextureType_SHININESS");
						break;
					case aiTextureType_OPACITY:
						Log::GetLogger()->info("  Semantic = aiTextureType_OPACITY");
						break;
					case aiTextureType_DISPLACEMENT:
						Log::GetLogger()->info("  Semantic = aiTextureType_DISPLACEMENT");
						break;
					case aiTextureType_LIGHTMAP:
						Log::GetLogger()->info("  Semantic = aiTextureType_LIGHTMAP");
						break;
					case aiTextureType_REFLECTION:
						Log::GetLogger()->info("  Semantic = aiTextureType_REFLECTION");
						break;
					case aiTextureType_UNKNOWN:
						Log::GetLogger()->info("  Semantic = aiTextureType_UNKNOWN");
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
							Log::GetLogger()->info("    Metalness map path = {0}", texturePath);

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
			Log::GetLogger()->info("------------------------");
		}

		Log::GetLogger()->info("Hazel::MeshAnimPBR: Creating a Vertex Array...");

		m_VertexArray = new OpenGLVertexArray();

		Log::GetLogger()->info("Hazel::MeshAnimPBR: Creating a Vertex Buffer...");

		m_VertexBuffer = new OpenGLVertexBuffer(m_AnimatedVertices.data(), (uint32_t)m_AnimatedVertices.size() * sizeof(AnimatedVertex));
		m_VertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Int4, "a_BoneIDs" },
			{ ShaderDataType::Float4, "a_BoneWeights" },
			});
		m_VertexArray->AddVertexBuffer(m_VertexBuffer);

		Log::GetLogger()->info("Hazel::MeshAnimPBR: Creating an Index Buffer...");

		m_IndexBuffer = new OpenGLIndexBuffer(m_Indices.data(), (uint32_t)m_Indices.size() * sizeof(Index));
		m_VertexArray->SetIndexBuffer(m_IndexBuffer);

		Log::GetLogger()->info("Hazel::MeshAnimPBR: Total vertices: {0}", m_AnimatedVertices.size());
		Log::GetLogger()->info("Hazel::MeshAnimPBR: Total indices: {0}", m_Indices.size());
	}

	MeshAnimPBR::~MeshAnimPBR()
	{
		for (Material* material : m_Materials)
			delete material;

		for (Texture* texture : m_Textures)
			delete texture;

		for (Submesh* submesh : m_Submeshes)
			delete submesh;

		delete m_IndexBuffer;
		delete m_VertexBuffer;
		delete m_VertexArray;
	}

	void MeshAnimPBR::OnUpdate(float ts, bool debug)
	{
		m_WorldTime += ts;

		if (m_IsAnimated && m_Scene->mAnimations && m_AnimationPlaying)
		{

			float ticksPerSecond = (float)(m_Scene->mAnimations[0]->mTicksPerSecond != 0 ? m_Scene->mAnimations[0]->mTicksPerSecond : 25.0f) * m_TimeMultiplier;
			m_AnimationTime += ts * ticksPerSecond;
			m_AnimationTime = fmod(m_AnimationTime, (float)m_Scene->mAnimations[0]->mDuration);

			//	if (debug) {
			//		Log::GetLogger()->info("MeshAnimPBR::OnUpdate ts: {0} m_AnimationTime: {1} mDuration {2} ticksPerSecond {3}",
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

	void MeshAnimPBR::TraverseNodes(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 transform = parentTransform * Math::Mat4FromAssimpMat4(node->mTransformation);
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			uint32_t mesh = node->mMeshes[i];
			Submesh* submesh = m_Submeshes[mesh];
			submesh->NodeName = node->mName.C_Str();
			submesh->Transform = transform;
		}

		Log::GetLogger()->info("{0} {1}", LevelToSpaces(level), node->mName.C_Str());

		for (uint32_t i = 0; i < node->mNumChildren; i++)
			TraverseNodes(node->mChildren[i], transform, level + 1);
	}

	uint32_t MeshAnimPBR::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
		{
			if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
				return i;
		}

		return 0;
	}

	uint32_t MeshAnimPBR::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
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


	uint32_t MeshAnimPBR::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
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


	glm::vec3 MeshAnimPBR::InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim)
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


	glm::quat MeshAnimPBR::InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim)
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


	glm::vec3 MeshAnimPBR::InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim)
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

	void MeshAnimPBR::SetupDefaultBaseMaterial()
	{
		// Setup default Material
		TextureInfo textureInfoDefault = {};
		textureInfoDefault.albedo    = "Textures/plain.png";
		textureInfoDefault.normal    = "Textures/normal_map_default.png";
		textureInfoDefault.metallic  = "Textures/plain.png";
		textureInfoDefault.roughness = "Textures/plain.png";
		textureInfoDefault.ao        = "Textures/plain.png";
		m_BaseMaterial = new Material(textureInfoDefault, 0.0f, 0.0f);
	}

	Texture* MeshAnimPBR::LoadBaseTexture()
	{
		if (!m_BaseTexture) {
			m_BaseTexture = new Texture("Textures/plain.png");
		}

		return m_BaseTexture;
	}

	void MeshAnimPBR::ImGuiNodeHierarchy(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 localTransform = Math::Mat4FromAssimpMat4(node->mTransformation);
		glm::mat4 transform = parentTransform * localTransform;

		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			uint32_t mesh = node->mMeshes[i];
			m_Submeshes[mesh]->NodeName = node->mName.C_Str();
			m_Submeshes[mesh]->Transform = transform;
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

	void MeshAnimPBR::OnImGuiRender()
	{
		// Mesh Hierarchy
		ImGui::Begin("Mesh Hierarchy");
		ImGuiNodeHierarchy(m_Scene->mRootNode, glm::mat4(1.0f), 0);
		ImGui::End();

		ImGui::Begin("Mesh Debug");
		if (ImGui::CollapsingHeader(m_FilePath.c_str()))
		{
			if (m_IsAnimated && m_Scene->mAnimations)
			{
				if (ImGui::CollapsingHeader("Animation"))
				{
					if (ImGui::Button(m_AnimationPlaying ? "Pause" : "Play"))
						m_AnimationPlaying = !m_AnimationPlaying;

					ImGui::SliderFloat("##AnimationTime", &m_AnimationTime, 0.0f, (float)m_Scene->mAnimations[0]->mDuration);
					ImGui::DragFloat("Time Scale", &m_TimeMultiplier, 0.05f, 0.0f, 100.0f);
				}
			}
		}
		ImGui::End();
	}

	void MeshAnimPBR::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& parentTransform)
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

	aiNodeAnim* MeshAnimPBR::FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
	{
		for (uint32_t i = 0; i < animation->mNumChannels; i++)
		{
			aiNodeAnim* nodeAnim = animation->mChannels[i];
			if (std::string(nodeAnim->mNodeName.data) == nodeName)
				return nodeAnim;
		}
		return nullptr;
	}

	void MeshAnimPBR::BoneTransform(float time)
	{
		ReadNodeHierarchy(time, m_Scene->mRootNode, glm::mat4(1.0f));
		m_BoneTransforms.resize(m_BoneCount);
		for (size_t i = 0; i < m_BoneCount; i++)
			m_BoneTransforms[i] = m_BoneInfo[i].FinalTransformation;
	}

	void MeshAnimPBR::DumpVertexBuffer()
	{
		// TODO: Convert to ImGui
		Log::GetLogger()->info("------------------------------------------------------");
		Log::GetLogger()->info("Vertex Buffer Dump");
		Log::GetLogger()->info("Mesh: {0}", m_FilePath);
		for (size_t i = 0; i < m_AnimatedVertices.size(); i++)
		{
			auto& vertex = m_AnimatedVertices[i];
			Log::GetLogger()->info("Vertex:   {0}", i);
			Log::GetLogger()->info("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
			Log::GetLogger()->info("Normal:   {0}, {1}, {2}", vertex.Normal.x,   vertex.Normal.y,   vertex.Normal.z);
			Log::GetLogger()->info("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
			Log::GetLogger()->info("Tangent:  {0}, {1}, {2}", vertex.Tangent.x,  vertex.Tangent.y,  vertex.Tangent.z);
			Log::GetLogger()->info("TexCoord: {0}, {1}",      vertex.Texcoord.x, vertex.Texcoord.y);
			Log::GetLogger()->info("--");
		}
		Log::GetLogger()->info("------------------------------------------------------");
	}

	void MeshAnimPBR::Render(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, EnvMapMaterial*>& envMapMaterials)
	{
		EnvMapMaterial* envMapMaterial = nullptr;

		m_VertexArray->Bind();

		for (Submesh* submesh : m_Submeshes)
		{
			m_MeshShader->Bind();

			for (size_t i = 0; i < m_BoneTransforms.size(); i++)
			{
				std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
				m_MeshShader->setMat4(uniformName, m_BoneTransforms[i]);
			}

			m_MeshShader->setMat4("u_Transform", transform * submesh->Transform);

			// Manage materials (PBR texture binding)
			if (m_BaseMaterial) {
				m_BaseMaterial->GetTextureAlbedo()->Bind(samplerSlot + 0);
				m_BaseMaterial->GetTextureNormal()->Bind(samplerSlot + 1);
				m_BaseMaterial->GetTextureMetallic()->Bind(samplerSlot + 2);
				m_BaseMaterial->GetTextureRoughness()->Bind(samplerSlot + 3);
				m_BaseMaterial->GetTextureAO()->Bind(samplerSlot + 4);
			}

			if (envMapMaterials.contains(submesh->NodeName))
			{
				envMapMaterial = envMapMaterials.at(submesh->NodeName);
				envMapMaterial->GetAlbedoInput().TextureMap->Bind(samplerSlot + 0);
				envMapMaterial->GetNormalInput().TextureMap->Bind(samplerSlot + 1);
				envMapMaterial->GetMetalnessInput().TextureMap->Bind(samplerSlot + 2);
				envMapMaterial->GetRoughnessInput().TextureMap->Bind(samplerSlot + 3);
				envMapMaterial->GetAOInput().TextureMap->Bind(samplerSlot + 4);
			}

			auto material = m_Materials[submesh->MaterialIndex];
			if (material->GetFlag(MaterialFlag::DepthTest)) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}

			glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t)* submesh->BaseIndex), submesh->BaseVertex);
		}
	}

	void MeshAnimPBR::RenderSubmeshes(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, EnvMapMaterial*>& envMapMaterials)
	{
		for (Hazel::Submesh* submesh : m_Submeshes)
		{
			submesh->Render(this, m_MeshShader, transform, samplerSlot, envMapMaterials);
		}
	}

	void Submesh::Render(MeshAnimPBR* parentMesh, Shader* shader, glm::mat4 transform, uint32_t samplerSlot,
		const std::map<std::string, EnvMapMaterial*>& envMapMaterials)
	{
		EnvMapMaterial* envMapMaterial = nullptr;

		parentMesh->GetVertexArray().Bind();

		shader->Bind();

		for (size_t i = 0; i < parentMesh->GetBoneTransforms().size(); i++)
		{
			std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
			shader->setMat4(uniformName, parentMesh->GetBoneTransforms()[i]);
		}

		shader->setMat4("u_Transform", transform * Transform);

		// Manage materials (PBR texture binding)
		if (m_BaseMaterial) {
			m_BaseMaterial->GetTextureAlbedo()->Bind(samplerSlot + 0);
			m_BaseMaterial->GetTextureNormal()->Bind(samplerSlot + 1);
			m_BaseMaterial->GetTextureMetallic()->Bind(samplerSlot + 2);
			m_BaseMaterial->GetTextureRoughness()->Bind(samplerSlot + 3);
			m_BaseMaterial->GetTextureAO()->Bind(samplerSlot + 4);
		}

		if (envMapMaterials.contains(NodeName))
		{
			envMapMaterial = envMapMaterials.at(NodeName);
			envMapMaterial->GetAlbedoInput().TextureMap->Bind(samplerSlot + 0);
			envMapMaterial->GetNormalInput().TextureMap->Bind(samplerSlot + 1);
			envMapMaterial->GetMetalnessInput().TextureMap->Bind(samplerSlot + 2);
			envMapMaterial->GetRoughnessInput().TextureMap->Bind(samplerSlot + 3);
			envMapMaterial->GetAOInput().TextureMap->Bind(samplerSlot + 4);
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
