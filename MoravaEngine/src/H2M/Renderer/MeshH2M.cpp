#define _CRT_SECURE_NO_WARNINGS

#include "MeshH2M.h"

// #include <GL/glew.h>

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

#include "H2M/Scene/EntityH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RendererH2M.h"

// TEMPORARY VULKAN INCLUDES
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"  // TODO: to be removed from MeshH2M
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h" // TODO: to be removed from MeshH2M
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"   // TODO: to be removed from MeshH2M
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"  // TODO: to be removed from MeshH2M

#include "Core/Log.h"
#include "Core/Math.h"
#include "Core/Util.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "Material/MaterialLibrary.h"
#include "Shader/MoravaShaderLibrary.h"

#include "imgui.h"

#include <filesystem>


namespace H2M
{

#define MESH_DEBUG_LOG 1
#if MESH_DEBUG_LOG
	#define HZ_MESH_LOG(...) MORAVA_CORE_TRACE(__VA_ARGS__)
#else
	#define HZ_MESH_LOG(...)
#endif

	static VkDescriptorSet s_DescriptorSet;
	static std::vector<VkWriteDescriptorSet> s_WriteDescriptorSets;

	// glm::mat4 Mat4FromAssimpMat4(const aiMatrix4x4& matrix) moved to Math class

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

	MeshH2M::MeshH2M(const std::string& filename)
	{
		m_FilePath = filename;
		m_BaseTexture = nullptr;
		m_MeshShader = nullptr;
		m_BaseMaterial = nullptr;
		m_IsAnimated = false;

		Create();
	}

	MeshH2M::MeshH2M(const std::string& filename, RefH2M<MoravaShader> shader, RefH2M<MaterialH2M> material, bool isAnimated)
		: m_MeshShader(shader), m_BaseMaterial(material), m_IsAnimated(isAnimated)
	{
		m_FilePath = filename;

		if (!m_BaseMaterial)
		{
			SetupDefaultBaseMaterial();
		}

		m_BaseTexture = nullptr;

		Create();
	}

	//	MeshH2M::MeshH2M(const std::string& filename)
	//		: m_FilePath(filename)
	//	{
	//		Create();
	//	}

	void MeshH2M::Create()
	{
		LogStream::Initialize();

		Log::GetLogger()->info("Hazel::MeshH2M: Loading mesh: {0}", m_FilePath.c_str());

		m_Importer = std::make_unique<Assimp::Importer>();

		const aiScene* scene = m_Importer->ReadFile(m_FilePath, s_MeshImportFlags);
		if (!scene || !scene->HasMeshes()) {
			Log::GetLogger()->error("Failed to load mesh file: {0}", m_FilePath);
			return;
		}

		m_Scene = scene;

		m_IsAnimated = scene->mAnimations != nullptr;
		// m_MaterialInstance = std::make_shared<MaterialInstance>(m_BaseMaterial);

		if (!m_MeshShader)
		{
			/**** BEGIN MoravaShader the new API ****/
			if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
			{
				MoravaShaderSpecification moravaShaderSpecificationStatic;
				moravaShaderSpecificationStatic.ShaderType = MoravaShaderSpecification::ShaderType::MoravaShader;
				moravaShaderSpecificationStatic.VertexShaderPath = "Shaders/Hazel/HazelPBR_Static.vs";
				moravaShaderSpecificationStatic.FragmentShaderPath = "Shaders/Hazel/HazelPBR.fs";
				// moravaShaderSpecificationStatic.FragmentShaderPath = "Shaders/Hazel/HazelPBR_Static_CSM.fs"; // fragment shader with Cascaded Shadow Maps
				moravaShaderSpecificationStatic.ForceCompile = false;

				MoravaShaderSpecification moravaShaderSpecificationAnim;
				moravaShaderSpecificationAnim.ShaderType = MoravaShaderSpecification::ShaderType::MoravaShader;
				moravaShaderSpecificationAnim.VertexShaderPath = "Shaders/Hazel/HazelPBR_Anim.vs";
				moravaShaderSpecificationAnim.FragmentShaderPath = "Shaders/Hazel/HazelPBR.fs";
				moravaShaderSpecificationAnim.ForceCompile = false;

				m_MeshShader = m_IsAnimated ? MoravaShader::Create(moravaShaderSpecificationAnim) : MoravaShader::Create(moravaShaderSpecificationStatic);
			}
			else if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::Vulkan)
			{
				MoravaShaderSpecification moravaShaderSpecificationHazelVulkan;
				moravaShaderSpecificationHazelVulkan.ShaderType = MoravaShaderSpecification::ShaderType::HazelShader;
				moravaShaderSpecificationHazelVulkan.HazelShaderPath = "assets/shaders/HazelPBR_Static.glsl";
				moravaShaderSpecificationHazelVulkan.ForceCompile = true;

				m_MeshShader = MoravaShader::Create(moravaShaderSpecificationHazelVulkan);
			}
			else if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::DX11)
			{
				MoravaShaderSpecification moravaShaderSpecificationHazelDX11;
				moravaShaderSpecificationHazelDX11.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
				moravaShaderSpecificationHazelDX11.VertexShaderPath = "Shaders/HLSL/BasicVertexShader.hlsl";
				moravaShaderSpecificationHazelDX11.PixelShaderPath = "Shaders/HLSL/BasicPixelShader.hlsl";
				moravaShaderSpecificationHazelDX11.ForceCompile = false;
				m_MeshShader = ResourceManager::CreateOrLoadShader(moravaShaderSpecificationHazelDX11);
			}
			/**** END MoravaShader the new API ****/
		}

		m_InverseTransform = glm::inverse(Math::Mat4FromAssimpMat4(scene->mRootNode->mTransformation));

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		Log::GetLogger()->info("Hazel::MeshH2M: Master mesh contains {0} submeshes.", scene->mNumMeshes);

		m_Submeshes.reserve(scene->mNumMeshes);
		for (size_t m = 0; m < scene->mNumMeshes; m++)
		{
			aiMesh* mesh = scene->mMeshes[m];

			RefH2M<SubmeshH2M> submesh = RefH2M<SubmeshH2M>::Create();
			submesh->BaseVertex = vertexCount;
			submesh->BaseIndex = indexCount;
			submesh->MaterialIndex = mesh->mMaterialIndex;
			submesh->IndexCount = mesh->mNumFaces * 3;
			submesh->MeshName = mesh->mName.C_Str();
			// m_Submeshes.push_back(submesh);

			vertexCount += mesh->mNumVertices;
			indexCount += submesh->IndexCount;

			H2M_CORE_ASSERT(mesh->HasPositions(), "Meshes require positions.");
			H2M_CORE_ASSERT(mesh->HasNormals(), "Meshes require normals.");

			// Vertices
			if (m_IsAnimated)
			{
				auto& aabb = submesh->BoundingBox;
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
					{
						vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
					}

					m_AnimatedVertices.push_back(vertex);
				}
			}
			else
			{
				auto& aabb = submesh->BoundingBox;
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
					{
						vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
					}

					m_StaticVertices.push_back(vertex);
				}
			}

			// Indices
			for (size_t i = 0; i < mesh->mNumFaces; i++)
			{
				// HZ_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices.");
				if (mesh->mFaces[i].mNumIndices != 3)
				{
					Log::GetLogger()->error("MeshH2M: the face contains invalid number of indices (expected: 3, detected: {0})!", mesh->mFaces[i].mNumIndices);
					continue;
				}

				Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
				m_Indices.push_back(index);

				// Triangle cache
				if (!m_IsAnimated)
				{
					if (index.V1 + submesh->BaseVertex < m_StaticVertices.size() &&
						index.V2 + submesh->BaseVertex < m_StaticVertices.size() &&
						index.V3 + submesh->BaseVertex < m_StaticVertices.size())
					{
						m_TriangleCache[(uint32_t)m].emplace_back(
							m_StaticVertices[index.V1 + submesh->BaseVertex],
							m_StaticVertices[index.V2 + submesh->BaseVertex],
							m_StaticVertices[index.V3 + submesh->BaseVertex]);
					}
				}
			}
		}

		// Display the list of all submeshes
		for (size_t m = 0; m < scene->mNumMeshes; m++) {
			Log::GetLogger()->info("-- Submesh ID {0} NodeName: '{1}'", m, m_Submeshes[m]->NodeName);
		}

		TraverseNodes(scene->mRootNode);

		PipelineSpecificationH2M pipelineSpecification;
		auto shader = m_MeshShader; // MoravaShader::Create("assets/shaders/VulkanWeekMesh.glsl", true);
		pipelineSpecification.Shader = m_MeshShader;

		//	if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
		//	{
		//		// HazelRenderer::Submit([instance, shader]() mutable
		//		// {
		//		// });
		//		{
		//			s_DescriptorSet = m_MeshShader.As<VulkanShader>()->CreateDescriptorSet(); // depends on m_DescriptorPool and m_DescriptorSetLayout
		//	
		//			/**** BEGIN Composite version 080a7edc, Sep 25th ****/
		//			// EXAMPLE:
		//			// std::vector<VkWriteDescriptorSet> writeDescriptorSets = Renderer::GetWriteDescriptorSet(pipelineSpecification.Shader);
		//			auto& ub0 = shader.As<VulkanShader>()->GetUniformBuffer(0);
		//			VkWriteDescriptorSet writeDescriptorSet = {};
		//			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//			writeDescriptorSet.dstSet = s_DescriptorSet;
		//			writeDescriptorSet.descriptorCount = 1;
		//			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		//			writeDescriptorSet.pBufferInfo = &ub0.Descriptor;
		//			writeDescriptorSet.dstBinding = 0;
		//			s_WriteDescriptorSets.push_back(writeDescriptorSet);
		//	
		//			auto& ub1 = shader.As<VulkanShader>()->GetUniformBuffer(1);
		//			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//			writeDescriptorSet.dstSet = s_DescriptorSet;
		//			writeDescriptorSet.descriptorCount = 1;
		//			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		//			writeDescriptorSet.pBufferInfo = &ub1.Descriptor;
		//			writeDescriptorSet.dstBinding = 1;
		//			s_WriteDescriptorSets.push_back(writeDescriptorSet);
		//			/**** END Composite 080a7edc, Sep 25th ****/
		//		}
		//	}

		// RefH2M<Mesh> instance = this;

		// Bones
		if (m_IsAnimated)
		{
			for (size_t m = 0; m < scene->mNumMeshes; m++)
			{
				aiMesh* mesh = scene->mMeshes[m];
				RefH2M<SubmeshH2M> submesh = m_Submeshes[m];

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
						if (m_AnimatedVertices.size() > VertexID) {
							m_AnimatedVertices[VertexID].AddBoneData(boneIndex, Weight);
						}
					}
				}
			}
		}

		/**** BEGIN Materials ****/

		// Materials
		if (scene->HasMaterials())
		{
			Log::GetLogger()->info("---- Materials - {0} ----", m_FilePath);

			m_MeshShader->Bind();

			m_Textures.resize(scene->mNumMaterials);
			m_Materials.resize(scene->mNumMaterials);
			m_MaterialDescriptors.resize(scene->mNumMaterials); // TODO: to be removed from MeshH2M

			RefH2M<Texture2D_H2M> whiteTexture = RendererH2M::GetWhiteTexture();

			for (uint32_t i = 0; i < scene->mNumMaterials; i++)
			{
				auto aiMaterial = scene->mMaterials[i];
				auto aiMaterialName = aiMaterial->GetName();

				auto mi = MaterialH2M::Create(m_MeshShader, aiMaterialName.data);
				// auto mi = HazelMaterial::Create(m_BaseMaterial, aiMaterialName.data);
				// auto mi = RefH2M<MaterialInstanceH2M>::Create(m_BaseMaterial, aiMaterialName.data);
				m_Materials[i] = mi;

				/**** BEGIN to be removed from MeshH2M ****/
				if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
				{
					// HazelRenderer::Submit([instance, shader, i]() mutable {});
					{
						MaterialDescriptor& materialDescriptor = m_MaterialDescriptors[i];
						materialDescriptor.DescriptorSet = shader.As<VulkanShaderH2M>()->CreateDescriptorSets();

						// EXAMPLE:
						// std::vector<VkWriteDescriptorSet> writeDescriptorSets = Renderer::GetWriteDescriptorSet(pipelineSpecification.Shader);
						auto& ub0 = shader.As<VulkanShaderH2M>()->GetUniformBuffer(0);
						VkWriteDescriptorSet writeDescriptorSet = {};
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.dstSet = *materialDescriptor.DescriptorSet.DescriptorSets.data();
						writeDescriptorSet.descriptorCount = (uint32_t)materialDescriptor.DescriptorSet.DescriptorSets.size();
						writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writeDescriptorSet.pBufferInfo = &ub0.Descriptor;
						writeDescriptorSet.dstBinding = 0;
						materialDescriptor.WriteDescriptors.push_back(writeDescriptorSet);

						auto& ub1 = shader.As<VulkanShaderH2M>()->GetUniformBuffer(1);
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.dstSet = *materialDescriptor.DescriptorSet.DescriptorSets.data();
						writeDescriptorSet.descriptorCount = (uint32_t)materialDescriptor.DescriptorSet.DescriptorSets.size();
						writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writeDescriptorSet.pBufferInfo = &ub1.Descriptor;
						writeDescriptorSet.dstBinding = 1;
						materialDescriptor.WriteDescriptors.push_back(writeDescriptorSet);
					}
				}
				/**** END to be removed from MeshH2M ****/

				Log::GetLogger()->info("  {0} (Index = {1})", aiMaterialName.data, i);
				aiString aiTexPath;
				uint32_t textureCount = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
				HZ_MESH_LOG("    TextureCount = {0}", textureCount);

				glm::vec3 albedoColor(0.8f);
				aiColor3D aiColor;
				// aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor);
				if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS)
				{
					albedoColor = { aiColor.r, aiColor.g, aiColor.b };
				}

				mi->Set("u_MaterialUniforms.AlbedoColor", albedoColor);

				float shininess, metalness;
				// aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
				if (aiMaterial->Get(AI_MATKEY_SHININESS, shininess) != aiReturn_SUCCESS)
				{
					shininess = 80.0f; // Default value
				}

				// aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness);
				if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS)
				{
					metalness = 0.0f;
				}

				// float roughness = 1.0f - shininess * 0.01f;
				// roughness *= roughness;
				float roughness = 1.0f - glm::sqrt(shininess / 100.0f);
				HZ_MESH_LOG("    COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);
				HZ_MESH_LOG("    ROUGHNESS = {0}", roughness);
				HZ_MESH_LOG("    METALNESS = {0}", metalness);

				// BEGIN the material data section
				RefH2M<SubmeshH2M> submeshPtr = RefH2M<SubmeshH2M>();
				if (i < m_Submeshes.size()) {
					submeshPtr = &m_Submeshes[i];
				}

				RefH2M<MaterialData> materialData = MaterialLibrary::AddNewMaterial(m_Materials[i], submeshPtr);
				// END the material data section

				bool hasAlbedoMap = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
				bool fallback = !hasAlbedoMap;

				if (hasAlbedoMap)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::filesystem::path path = m_FilePath;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();
					HZ_MESH_LOG("    Albedo map path = '{0}'", texturePath);

					RefH2M<Texture2D_H2M> texture = RefH2M<Texture2D_H2M>();
					try {
						// texture = Texture2D_H2M::Create(texturePath, false);
						texture = ResourceManager::LoadTexture2D_H2M(texturePath);
					}
					catch (...) {
						Log::GetLogger()->warn("The ALBEDO map failed to load. Loading the default texture placeholder instead.");
						texture = LoadBaseTexture();
					}

					if (texture && texture->Loaded())
					{
						m_Textures[i] = texture;
						mi->Set("u_AlbedoTexture", texture); //  VulkanMaterial::FindResourceDeclaration - no resources found (name 'u_AlbedoTexture')!
						// mi->Set("u_MaterialUniforms.UseAlbedoMap", true);

						if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
						{
							AddMaterialTextureWriteDescriptor(i, "u_AlbedoTexture", texture); // TODO: to be removed from MeshH2M
						}
						else
						{
							m_MeshShader->SetInt("u_AlbedoTexture", texture->GetID());
							mi->Set("u_AlbedoTexture", texture->GetID()); // redundant
						}

						m_MeshShader->SetFloat("u_MaterialUniforms.AlbedoTexToggle", 1.0f);
						mi->Set("u_MaterialUniforms.AlbedoTexToggle", 1.0f); // redundant
						MaterialLibrary::AddTextureToEnvMapMaterial(MaterialTextureType::Albedo, texturePath, materialData->EnvMapMaterialRef);
					}
					else
					{
						Log::GetLogger()->error("Could not load texture: {0}", texturePath);

						// Fallback to albedo color
						m_MeshShader->SetFloat3("u_MaterialUniforms.AlbedoColor", glm::vec3{ aiColor.r, aiColor.g, aiColor.b });
						mi->Set("u_MaterialUniforms.AlbedoColor", glm::vec3{ aiColor.r, aiColor.g, aiColor.b }); // redundant

						HZ_MESH_LOG("Mesh has no Albedo map.");

						fallback = true;
					}
				}
				else
				{
					m_MeshShader->SetFloat3("u_MaterialUniforms.AlbedoColor", glm::vec3{ aiColor.r, aiColor.g, aiColor.b });
					mi->Set("u_MaterialUniforms.AlbedoColor", glm::vec3{ aiColor.r, aiColor.g, aiColor.b }); // redundant
					Log::GetLogger()->info("    No albedo map");
				}

				if (fallback)
				{
					// HZ_MESH_LOG("    No albedo map");
					Log::GetLogger()->info("    No normal map");
					mi->Set("u_AlbedoTexture", whiteTexture);

					if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
					{
						AddMaterialTextureWriteDescriptor(i, "u_AlbedoTexture", whiteTexture); // TODO: to be removed from MeshH2M
					}
				}

				// Normal maps
				m_MeshShader->SetFloat("u_MaterialUniforms.NormalTexToggle", 0.0f);
				mi->Set("u_MaterialUniforms.NormalTexToggle", 0.0f); // redundant

				bool hasNormalMap = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;
				fallback = !hasNormalMap;

				if (hasNormalMap)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::filesystem::path path = m_FilePath;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();
					HZ_MESH_LOG("    Normal map path = '{0}'", texturePath);

					RefH2M<Texture2D_H2M> texture = RefH2M<Texture2D_H2M>();
					try {
						// texture = Texture2D_H2M::Create(texturePath, false);
						texture = ResourceManager::LoadTexture2D_H2M(texturePath);
						m_Textures.push_back(texture);
					}
					catch (...) {
						Log::GetLogger()->warn("The NORMAL map failed to load. Loading the default texture placeholder instead.");
						texture = Texture2D_H2M::Create("Textures/normal_map_default.png");
					}

					if (texture && texture->Loaded())
					{
						m_Textures.push_back(texture);

						mi->Set("u_NormalTexture", texture);
						// mi->Set("u_MaterialUniforms.UseNormalMap", true);

						if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
						{
							AddMaterialTextureWriteDescriptor(i, "u_NormalTexture", texture); // TODO: to be removed from MeshH2M
						}
						else
						{
							m_MeshShader->SetInt("u_NormalTexture", texture->GetID());
							mi->Set("u_NormalTexture", texture->GetID()); // redundant
						}

						m_MeshShader->SetFloat("u_MaterialUniforms.NormalTexToggle", 1.0f);
						mi->Set("u_MaterialUniforms.NormalTexToggle", 1.0f); // redundant
						MaterialLibrary::AddTextureToEnvMapMaterial(MaterialTextureType::Normal, texturePath, materialData->EnvMapMaterialRef);
					}
					else
					{
						Log::GetLogger()->error("    Could not load texture: {0}", texturePath);
						fallback = true;
					}
				}

				if (fallback)
				{
					// HZ_MESH_LOG("    No normal map");
					Log::GetLogger()->info("    No normal map");

					mi->Set("u_NormalTexture", whiteTexture);

					if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
					{
						AddMaterialTextureWriteDescriptor(i, "u_NormalTexture", whiteTexture); // TODO: to be removed from MeshH2M
					}
				}

				// Roughness map
				// m_MeshShader->SetFloat("u_MaterialUniforms.Roughness", 1.0f);
				// m_MeshShader->SetFloat("u_MaterialUniforms.RoughnessTexToggle", 0.0f);

				bool hasRoughnessMap = aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS;
				fallback = !hasRoughnessMap;

				if (aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::filesystem::path path = m_FilePath;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();
					// HZ_MESH_LOG("    Roughness map path = '{0}'", texturePath);
					HZ_MESH_LOG("    Roughness map path = '{0}'", texturePath);

					RefH2M<Texture2D_H2M> texture = RefH2M<Texture2D_H2M>();
					try {
						// texture = Texture2D_H2M::Create(texturePath, false);
						texture = ResourceManager::LoadTexture2D_H2M(texturePath);
					}
					catch (...) {
						Log::GetLogger()->warn("The ROUGHNESS map failed to load. Loading the default texture placeholder instead.");
						texture = LoadBaseTexture();
					}

					if (texture->Loaded())
					{
						m_Textures[i] = texture;
						mi->Set("u_RoughnessTexture", texture); // VulkanMaterial::FindResourceDeclaration - no resources found (name 'u_RoughnessTexture')!
						// mi->Set("u_MaterialUniforms.UseRoughnessMap", true); // VulkanMaterial::Set - Could not find uniform with name 'u_MaterialUniforms.UseRoughnessMap'!

						if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
						{
							AddMaterialTextureWriteDescriptor(i, "u_RoughnessTexture", texture); // TODO: to be removed from MeshH2M
						}
						else
						{
							m_MeshShader->SetInt("u_RoughnessTexture", texture->GetID());
							mi->Set("u_RoughnessTexture", texture->GetID()); // redundant
						}

						m_MeshShader->SetFloat("u_MaterialUniforms.RoughnessTexToggle", 1.0f);
						mi->Set("u_MaterialUniforms.RoughnessTexToggle", 1.0f); // redundant
						MaterialLibrary::AddTextureToEnvMapMaterial(MaterialTextureType::Roughness, texturePath, materialData->EnvMapMaterialRef);
					}
					else
					{
						Log::GetLogger()->error("    Could not load texture: {0}", texturePath);
						fallback = true;

						m_MeshShader->SetFloat("u_MaterialUniforms.Roughness", roughness);
						m_MeshShader->SetFloat("u_MaterialUniforms.RoughnessTexToggle", 0.0f);
					}
				}

				if (fallback)
				{
					Log::GetLogger()->info("    No roughness map");

					m_MeshShader->SetFloat("u_MaterialUniforms.Roughness", roughness);
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

					RefH2M<Texture2D_H2M> texture = RefH2M<Texture2D_H2M>();
					try {
						// texture = Texture2D_H2M::Create(texturePath, false);
						texture = ResourceManager::LoadTexture2D_H2M(texturePath);
					}
					catch (...) {
						Log::GetLogger()->warn("The METALNESS map failed to load. Loading the default texture placeholder instead.");
						texture = LoadBaseTexture();
					}

					if (texture->Loaded())
					{
						HZ_MESH_LOG("    Metalness map path = '{0}'", texturePath);
						m_MeshShader->SetInt("u_MetalnessTexture", texture->GetID());
						m_MeshShader->SetFloat("u_MaterialUniforms.MetalnessTexToggle", 1.0f);
					}
					else
					{
						Log::GetLogger()->error("Could not load texture: {0}", texturePath);
					}
				}
				else
				{
					Log::GetLogger()->info("    No metalness texture");
					m_MeshShader->SetFloat("u_MaterialUniforms.Metalness", metalness);
				}
#endif

				bool metalnessTextureFound = false;
				for (uint32_t p = 0; p < aiMaterial->mNumProperties; p++)
				{
					auto prop = aiMaterial->mProperties[p];

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
							// TODO: Temp - this should be handled by Hazel's filesystem
							std::filesystem::path path = m_FilePath;
							auto parentPath = path.parent_path();
							parentPath /= str;
							std::string texturePath = parentPath.string();
							HZ_MESH_LOG("    Metalness map path = '{0}'", texturePath);

							RefH2M<Texture2D_H2M> texture = RefH2M<Texture2D_H2M>();
							try {
								// texture = Texture2D_H2M::Create(texturePath, false);
								texture = ResourceManager::LoadTexture2D_H2M(texturePath);
							}
							catch (...) {
								Log::GetLogger()->warn("The METALNESS map failed to load. Loading the default texture placeholder instead.");
								texture = LoadBaseTexture();
							}

							if (texture->Loaded())
							{
								metalnessTextureFound = true;

								m_Textures.push_back(texture);
								mi->Set("u_MetalnessTexture", texture); // VulkanMaterial::FindResourceDeclaration - no resources found (name 'u_MetalnessTexture')!
								// mi->Set("u_MaterialUniforms.UseMetalnessMap", true); // VulkanMaterial::Set - Could not find uniform with name 'u_MaterialUniforms.UseMetalnessMap'!

								if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
								{
									AddMaterialTextureWriteDescriptor(0, "u_MetalnessTexture", texture); // TODO: to be removed from MeshH2M
								}
								else
								{
									m_MeshShader->SetInt("u_MetalnessTexture", texture->GetID());
									mi->Set("u_MetalnessTexture", texture->GetID()); // redundant
								}

								m_MeshShader->SetFloat("u_MaterialUniforms.MetalnessTexToggle", 1.0f);
								mi->Set("u_MaterialUniforms.MetalnessTexToggle", 1.0f); // redundant
								MaterialLibrary::AddTextureToEnvMapMaterial(MaterialTextureType::Metalness, texturePath, materialData->EnvMapMaterialRef);
							}
							break;
						}
					}
				}

				if (!metalnessTextureFound)
				{
					Log::GetLogger()->info("    No metalness map");
					// Log::GetLogger()->error("    Could not load texture: {0}", texturePath);

					m_MeshShader->SetFloat("u_MaterialUniforms.Metalness", metalness);
					m_MeshShader->SetFloat("u_MaterialUniforms.MetalnessTexToggle", 0.0f);
				}

				fallback = !metalnessTextureFound;
				if (fallback)
				{
					HZ_MESH_LOG("    No metalness map");

					mi->Set("u_MetalnessTexture", whiteTexture);

					if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
					{
						AddMaterialTextureWriteDescriptor(i, "u_MetalnessTexture", whiteTexture); // TODO: to be removed from MeshH2M
					}
				}
			}
			HZ_MESH_LOG("------------------------");
		}
		else
		{
			// auto mi = HazelMaterial::Create(m_MeshShader, aiMaterialName.data);
			auto mi = RefH2M<MaterialInstanceH2M>::Create(m_BaseMaterial, "Hazel-Default");
			mi->Set("u_MaterialUniforms.AlbedoTexToggle", 0.0f);
			mi->Set("u_MaterialUniforms.NormalTexToggle", 0.0f);
			mi->Set("u_MaterialUniforms.MetalnessTexToggle", 0.0f);
			mi->Set("u_MaterialUniforms.RoughnessTexToggle", 0.0f);
			mi->Set("u_MaterialUniforms.AlbedoColor", glm::vec3(0.8f, 0.1f, 0.3f));
			mi->Set("u_MaterialUniforms.Metalness", 0.0f);
			mi->Set("u_MaterialUniforms.Roughness", 0.8f);
			m_Materials.push_back(mi);
		}

		/**** END Materials ****/

		Log::GetLogger()->info("Hazel::MeshH2M: Creating a Vertex Buffer...");

		if (m_IsAnimated)
		{
			m_VertexBuffer = VertexBufferH2M::Create(m_AnimatedVertices.data(), (uint32_t)sizeof(AnimatedVertex), (uint32_t)m_AnimatedVertices.size());

			m_VertexBufferLayout = {
							{ ShaderDataTypeH2M::Float3, "a_Position" },
							{ ShaderDataTypeH2M::Float3, "a_Normal" },
							{ ShaderDataTypeH2M::Float3, "a_Tangent" },
							{ ShaderDataTypeH2M::Float3, "a_Binormal" },
							{ ShaderDataTypeH2M::Float2, "a_TexCoord" },
							{ ShaderDataTypeH2M::Int4,   "a_BoneIDs" },
							{ ShaderDataTypeH2M::Float4, "a_BoneWeights" },
			};
		}
		else
		{
			m_VertexBuffer = VertexBufferH2M::Create(m_StaticVertices.data(), (uint32_t)sizeof(Vertex), (uint32_t)m_StaticVertices.size());

			m_VertexBufferLayout = {
							{ ShaderDataTypeH2M::Float3, "a_Position" },
							{ ShaderDataTypeH2M::Float3, "a_Normal" },
							{ ShaderDataTypeH2M::Float3, "a_Tangent" },
							{ ShaderDataTypeH2M::Float3, "a_Binormal" },
							{ ShaderDataTypeH2M::Float2, "a_TexCoord" },
			};
		}

		Log::GetLogger()->info("Hazel::MeshH2M: Creating an Index Buffer...");
		m_IndexBuffer = IndexBufferH2M::Create(m_Indices.data(), (uint32_t)m_Indices.size() * sizeof(Index));

		/**** BEGIN Create pipeline ****/
		{
			// Temporary and only for OpenGL.
			// In Vulkan, the Pipeline is created in VulkanRenderer
			Log::GetLogger()->info("Hazel::MeshH2M: Creating a Pipeline...");

			pipelineSpecification.Layout = m_VertexBufferLayout;
			RenderPassSpecificationH2M renderPassSpec = {};
			pipelineSpecification.RenderPass = RenderPassH2M::Create(renderPassSpec);
			m_Pipeline = PipelineH2M::Create(pipelineSpecification);
		}
		/**** END Create pipeline ****/

		if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
		{
			// HazelRenderer::Submit([&]()
			// {
			// });
			{
				// auto vulkanDevice = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
				// // MORAVA_CORE_WARN("Updating {0} descriptor sets", s_WriteDescriptorSets.size());
				// Log::GetLogger()->warn("Updating {0} descriptor sets", s_WriteDescriptorSets.size());
				// vkUpdateDescriptorSets(vulkanDevice, static_cast<uint32_t>(s_WriteDescriptorSets.size()), s_WriteDescriptorSets.data(), 0, nullptr);
			}

			// vulkan branch, february 2021
			UpdateAllDescriptorSets();
		}

		size_t totalVertices = m_IsAnimated ? m_AnimatedVertices.size() : m_StaticVertices.size();

		Log::GetLogger()->info("Hazel::MeshH2M: Total vertices: {0}", totalVertices);
		Log::GetLogger()->info("Hazel::MeshH2M: Total indices: {0}", m_Indices.size());
	}

	MeshH2M::~MeshH2M()
	{
	}

	void MeshH2M::AddMaterialTextureWriteDescriptor(uint32_t index, const std::string& name, RefH2M<Texture2D_H2M> texture)
	{
		// RefH2M<MeshH2M> instance = this;
		// HazelRenderer::Submit([instance, index, name, texture]() mutable {});
		{
			MaterialDescriptor& materialDescriptor = m_MaterialDescriptors[index];

			RefH2M<ShaderH2M> shader = m_Materials[index]->GetShader();
			const VkWriteDescriptorSet* wds = shader.As<VulkanShaderH2M>()->GetDescriptorSet(name);
			H2M_CORE_ASSERT(wds);

			VkWriteDescriptorSet descriptorSet = *wds;
			descriptorSet.dstSet = *materialDescriptor.DescriptorSet.DescriptorSets.data();
			descriptorSet.descriptorCount = (uint32_t)materialDescriptor.DescriptorSet.DescriptorSets.size();
			auto& imageInfo = texture.As<VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
			descriptorSet.pImageInfo = &imageInfo;
			materialDescriptor.WriteDescriptors.push_back(descriptorSet);
		}
	}

	void MeshH2M::UpdateAllDescriptorSets()
	{
		// RefH2M<Mesh> instance = this;
		// HazelRenderer::Submit([instance]() mutable {});
		{
			auto vulkanDevice = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
			for (MaterialDescriptor& md : m_MaterialDescriptors)
			{
				// HZ_CORE_WARN("Updating {0} descriptor sets", md.WriteDescriptors.size());
				Log::GetLogger()->warn("Updating {0} descriptor sets", md.WriteDescriptors.size());
				vkUpdateDescriptorSets(vulkanDevice, (uint32_t)md.WriteDescriptors.size(), md.WriteDescriptors.data(), 0, nullptr);
			}
		}
	}

	/**** BEGIN removed in Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****
	void MeshH2M::UpdateAllDescriptors()
	{
		// RefH2M<MeshH2M> instance = this;
		// HazelRenderer::Submit([instance]() mutable {});
		{
			auto vulkanDevice = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
			for (MaterialDescriptor& md : m_MaterialDescriptors)
			{
				HZ_CORE_WARN("Updating {0} descriptor sets", md.WriteDescriptors.size());
				vkUpdateDescriptorSets(vulkanDevice, (uint32_t)md.WriteDescriptors.size(), md.WriteDescriptors.data(), 0, nullptr);
			}
		}
	}
	/**** END removed in Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****/

	void MeshH2M::OnUpdate(TimestepH2M ts, bool debug)
	{
		if (m_IsAnimated)
		{
			if (m_AnimationPlaying)
			{
				m_WorldTime += ts;

				float ticksPerSecond = (float)(m_Scene->mAnimations[0]->mTicksPerSecond != 0 ? m_Scene->mAnimations[0]->mTicksPerSecond : 25.0f) * m_TimeMultiplier;
				m_AnimationTime += ts * ticksPerSecond;
				m_AnimationTime = fmod(m_AnimationTime, (float)m_Scene->mAnimations[0]->mDuration);
			}

			// TODO: We only need to recalc bones if rendering has been requested at the current animation frame
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

	void MeshH2M::TraverseNodes(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 transform = parentTransform * Math::Mat4FromAssimpMat4(node->mTransformation);
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			uint32_t mesh = node->mMeshes[i];
			RefH2M<SubmeshH2M> submesh = m_Submeshes[mesh];
			submesh->NodeName = node->mName.C_Str();
			submesh->MeshName = m_Scene->mMeshes[mesh]->mName.C_Str();
			submesh->Transform = transform;
		}

		HZ_MESH_LOG("{0} {1}", LevelToSpaces(level), node->mName.C_Str());

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			TraverseNodes(node->mChildren[i], transform, level + 1);
		}
	}

	uint32_t MeshH2M::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
		{
			if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
			{
				return i;
			}
		}

		return 0;
	}

	uint32_t MeshH2M::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		if (pNodeAnim->mNumRotationKeys <= 0)
		{
			Log::GetLogger()->error("pNodeAnim->mNumRotationKeys <= 0");
		}

		for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
		{
			if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
			{
				return i;
			}
		}

		return 0;
	}

	uint32_t MeshH2M::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		if (pNodeAnim->mNumScalingKeys <= 0)
		{
			Log::GetLogger()->error("pNodeAnim->mNumScalingKeys <= 0");
		}

		for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
		{
			if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
			{
				return i;
			}
		}

		return 0;
	}

	glm::vec3 MeshH2M::InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim)
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
		{
			Factor = 0.0f;
		}

		if (Factor > 1.0f)
		{
			Log::GetLogger()->error("Factor must be below 1.0f");
		}

		const aiVector3D& Start = nodeAnim->mPositionKeys[PositionIndex].mValue;
		const aiVector3D& End = nodeAnim->mPositionKeys[NextPositionIndex].mValue;
		aiVector3D Delta = End - Start;
		auto aiVec = Start + Factor * Delta;
		return { aiVec.x, aiVec.y, aiVec.z };
	}

	glm::quat MeshH2M::InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim)
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
		{
			Log::GetLogger()->error("NextRotationIndex >= nodeAnim->mNumRotationKeys");
		}

		float DeltaTime = (float)(nodeAnim->mRotationKeys[NextRotationIndex].mTime - nodeAnim->mRotationKeys[RotationIndex].mTime);
		float Factor = (animationTime - (float)nodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
		if (Factor < 0.0f)
		{
			Factor = 0.0f;
		}

		if (Factor > 1.0f)
		{
			Log::GetLogger()->error("Factor must be below 1.0f");
		}

		const aiQuaternion& StartRotationQ = nodeAnim->mRotationKeys[RotationIndex].mValue;
		const aiQuaternion& EndRotationQ = nodeAnim->mRotationKeys[NextRotationIndex].mValue;
		auto q = aiQuaternion();
		aiQuaternion::Interpolate(q, StartRotationQ, EndRotationQ, Factor);
		q = q.Normalize();
		return glm::quat(q.w, q.x, q.y, q.z);
	}

	glm::vec3 MeshH2M::InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim)
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
		{
			Log::GetLogger()->error("nextIndex >= nodeAnim->mNumScalingKeys");
		}

		float deltaTime = (float)(nodeAnim->mScalingKeys[nextIndex].mTime - nodeAnim->mScalingKeys[index].mTime);
		float factor = (animationTime - (float)nodeAnim->mScalingKeys[index].mTime) / deltaTime;
		if (factor < 0.0f)
		{
			factor = 0.0f;
		}

		if (factor > 1.0f)
		{
			Log::GetLogger()->error("Factor must be below 1.0f");
		}

		const auto& start = nodeAnim->mScalingKeys[index].mValue;
		const auto& end = nodeAnim->mScalingKeys[nextIndex].mValue;
		auto delta = end - start;
		auto aiVec = start + factor * delta;
		return { aiVec.x, aiVec.y, aiVec.z };
	}

	void MeshH2M::SetupDefaultBaseMaterial()
	{
		// Setup default Material
		TextureInfo textureInfoDefault = {};
		textureInfoDefault.albedo    = "Textures/plain.png";
		textureInfoDefault.normal    = "Textures/normal_map_default.png";
		textureInfoDefault.metallic  = "Textures/plain.png";
		textureInfoDefault.roughness = "Textures/plain.png";
		textureInfoDefault.emissive  = "Texture/plain.png";
		textureInfoDefault.ao        = "Textures/plain.png";
		m_BaseMaterial = RefH2M<Material>::Create(textureInfoDefault, 0.0f, 0.0f);
	}

	RefH2M<Texture2D_H2M> MeshH2M::LoadBaseTexture()
	{
		if (!m_BaseTexture) {
			try {
				m_BaseTexture = Texture2D_H2M::Create("Textures/plain.png");
			}
			catch (...) {
				Log::GetLogger()->warn("Failed to load the base texture!");
				m_BaseTexture = nullptr;
			}
		}

		return m_BaseTexture;
	}

	void MeshH2M::ImGuiNodeHierarchy(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
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
			{
				ImGuiNodeHierarchy(node->mChildren[i], transform, level + 1);
			}

			ImGui::TreePop();
		}
	}

	void MeshH2M::OnImGuiRender(uint32_t id, bool* p_open)
	{
		if (!m_Scene)
		{
			Log::GetLogger()->error("Mesh: Scene not initialized!");
			return;
		}

		// Mesh Hierarchy
		ImGui::Begin("Mesh Hierarchy", p_open);
		ImGuiNodeHierarchy(m_Scene->mRootNode, glm::mat4(1.0f), 0);
		ImGui::End();

		ImGui::Begin("Mesh Debug", p_open);
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

	void MeshH2M::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& parentTransform)
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
		{
			ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], transform);
		}
	}

	aiNodeAnim* MeshH2M::FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
	{
		for (uint32_t i = 0; i < animation->mNumChannels; i++)
		{
			aiNodeAnim* nodeAnim = animation->mChannels[i];
			if (std::string(nodeAnim->mNodeName.data) == nodeName)
			{
				return nodeAnim;
			}
		}
		return nullptr;
	}

	void MeshH2M::DeleteSubmesh(RefH2M<SubmeshH2M> submesh)
	{
		for (auto iterator = m_Submeshes.cbegin(); iterator != m_Submeshes.cend();)
		{
			if (iterator->Raw()->MeshName == submesh->MeshName)
			{
				iterator = m_Submeshes.erase(iterator++);
				Log::GetLogger()->debug("MeshH2M::DeleteSubmesh erase '{0}'", submesh->MeshName);
			}
			else
			{
				++iterator;
			}
		}
	}

	void MeshH2M::CloneSubmesh(RefH2M<SubmeshH2M> submesh)
	{
		EntitySelection::s_SelectionContext.clear();

		// Ref<SubmeshHazelLegacy> submeshCopy = Ref<SubmeshHazelLegacy>::Create(submesh);
		RefH2M<SubmeshH2M> submeshCopy = RefH2M<SubmeshH2M>::Create();
		std::string appendix = Util::randomString(2);
		submeshCopy->MeshName += "." + appendix;
		submeshCopy->NodeName += "." + appendix;
		m_Submeshes.push_back(submeshCopy);
	}

	void MeshH2M::BoneTransform(float time)
	{
		ReadNodeHierarchy(time, m_Scene->mRootNode, glm::mat4(1.0f));
		m_BoneTransforms.resize(m_BoneCount);
		for (size_t i = 0; i < m_BoneCount; i++)
		{
			m_BoneTransforms[i] = m_BoneInfo[i].FinalTransformation;
		}
	}

	void MeshH2M::DumpVertexBuffer()
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

	const std::vector<Triangle> MeshH2M::GetTriangleCache(uint32_t index) const
	{
		std::unordered_map<uint32_t, std::vector<Triangle>>::const_iterator entry = m_TriangleCache.find(index);

		if (index < m_TriangleCache.size() && entry != m_TriangleCache.end())
		{
			return entry->second;
		}

		return std::vector<Triangle>();
	}

	void MeshH2M::Render(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials)
	{
		RefH2M<EnvMapMaterial> envMapMaterial = RefH2M<EnvMapMaterial>();

		m_VertexBuffer->Bind();
		m_Pipeline->Bind();
		m_IndexBuffer->Bind();

		for (RefH2M<SubmeshH2M> submesh : m_Submeshes)
		{
			m_MeshShader->Bind();

			for (size_t i = 0; i < m_BoneTransforms.size(); i++)
			{
				std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");

				m_MeshShader->SetMat4(uniformName, m_BoneTransforms[i]);
			}

			m_MeshShader->SetMat4("u_Transform", transform * submesh->Transform);

			// Manage materials (PBR texture binding)
			if (m_BaseMaterial)
			{
				RefH2M<Material> baseMaterialRef = m_BaseMaterial;
				baseMaterialRef->GetTextureAlbedo()->Bind(samplerSlot + 0);
				baseMaterialRef->GetTextureNormal()->Bind(samplerSlot + 1);
				baseMaterialRef->GetTextureMetallic()->Bind(samplerSlot + 2);
				baseMaterialRef->GetTextureRoughness()->Bind(samplerSlot + 3);
				baseMaterialRef->GetTextureEmissive()->Bind(samplerSlot + 4);
				baseMaterialRef->GetTextureAO()->Bind(samplerSlot + 5);
			}

			RefH2M<MeshH2M> instance = this;
			std::string materialUUID = MaterialLibrary::GetSubmeshMaterialUUID(instance, submesh, EntityH2M{});

			if (envMapMaterials.find(materialUUID) != envMapMaterials.end())
			{
				envMapMaterial = envMapMaterials.at(materialUUID);
				envMapMaterial->GetAlbedoInput().TextureMap->Bind(samplerSlot + 0);
				envMapMaterial->GetNormalInput().TextureMap->Bind(samplerSlot + 1);
				envMapMaterial->GetMetalnessInput().TextureMap->Bind(samplerSlot + 2);
				envMapMaterial->GetRoughnessInput().TextureMap->Bind(samplerSlot + 3);
				envMapMaterial->GetEmissiveInput().TextureMap->Bind(samplerSlot + 4);
				envMapMaterial->GetAOInput().TextureMap->Bind(samplerSlot + 5);
			}

			// RefH2M<MaterialInstanceH2M> material = RefH2M<MaterialInstanceH2M>();
			RefH2M<MaterialH2M> material = RefH2M<MaterialH2M>();
			if (m_Materials.size())
			{
				material = m_Materials[submesh->MaterialIndex];
				if (material && material->GetFlag(MaterialFlagH2M::DepthTest))
				{
					RendererBasic::EnableDepthTest();
				}
			}
			else
			{
				RendererBasic::DisableDepthTest();
			}

			RendererBasic::DrawIndexed(submesh->IndexCount, 0, submesh->BaseVertex, (void*)(sizeof(uint32_t) * submesh->BaseIndex));
			// glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);
		}
	}

	void MeshH2M::RenderSubmeshes(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials, EntityH2M entity)
	{
		for (RefH2M<SubmeshH2M> submesh : m_Submeshes)
		{
			submesh->Render(this, m_MeshShader, transform, samplerSlot, envMapMaterials, entity);
		}
	}

	void SubmeshH2M::Render(RefH2M<MeshH2M> parentMesh, RefH2M<MoravaShader> shader, const glm::mat4& entityTransform, uint32_t samplerSlot,
		const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials, EntityH2M entity, bool wireframeEnabledScene, bool wireframeEnabledModel)
	{
		RefH2M<EnvMapMaterial> envMapMaterial = RefH2M<EnvMapMaterial>();

		parentMesh->GetVertexBuffer()->Bind();
		parentMesh->GetPipeline()->Bind();
		parentMesh->GetIndexBuffer()->Bind();

		// Manage materials (PBR texture binding)
		if (m_BaseMaterial)
		{
			m_BaseMaterial->GetTextureAlbedo()->Bind(samplerSlot + 0);
			m_BaseMaterial->GetTextureNormal()->Bind(samplerSlot + 1);
			m_BaseMaterial->GetTextureMetallic()->Bind(samplerSlot + 2);
			m_BaseMaterial->GetTextureRoughness()->Bind(samplerSlot + 3);
			m_BaseMaterial->GetTextureEmissive()->Bind(samplerSlot + 4);
			m_BaseMaterial->GetTextureAO()->Bind(samplerSlot + 5);
		}

		std::string materialUUID = MaterialLibrary::GetSubmeshMaterialUUID(parentMesh, this, entity);

		if (envMapMaterials.find(materialUUID) != envMapMaterials.end())
		{
			envMapMaterial = envMapMaterials.at(materialUUID);
			envMapMaterial->GetAlbedoInput().TextureMap->Bind(samplerSlot + 0);
			envMapMaterial->GetNormalInput().TextureMap->Bind(samplerSlot + 1);
			envMapMaterial->GetMetalnessInput().TextureMap->Bind(samplerSlot + 2);
			envMapMaterial->GetRoughnessInput().TextureMap->Bind(samplerSlot + 3);
			envMapMaterial->GetEmissiveInput().TextureMap->Bind(samplerSlot + 4);
			envMapMaterial->GetAOInput().TextureMap->Bind(samplerSlot + 5);
		}

		auto material = parentMesh->GetMaterials()[MaterialIndex];
		if (material->GetFlag(MaterialFlagH2M::DepthTest))
		{
			RendererBasic::EnableDepthTest();
		}
		else
		{
			RendererBasic::DisableDepthTest();
		}

		shader->Bind();

		for (size_t i = 0; i < parentMesh->GetBoneTransforms().size(); i++)
		{
			std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
			shader->SetMat4(uniformName, parentMesh->GetBoneTransforms()[i]);
		}

		shader->SetMat4("u_Transform", entityTransform * Transform);

		// ATM too complex logic for rendering polygons and/or wireframe

		if (wireframeEnabledScene)
		{
			shader->SetBool("u_WireframeMode.Enabled", false);
			RendererBasic::DrawIndexed(IndexCount, 0, BaseVertex, (void*)(sizeof(uint32_t) * BaseIndex));
		}
		else
		{
			shader->SetBool("u_WireframeMode.Enabled", false);
			RendererBasic::SetPolygonMode(RendererBasic::PolygonMode::FILL);
			RendererBasic::DrawIndexed(IndexCount, 0, BaseVertex, (void*)(sizeof(uint32_t) * BaseIndex));

			if (wireframeEnabledModel)
			{
				shader->SetBool("u_WireframeMode.Enabled", true);
				shader->SetFloat4("u_WireframeMode.LineColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));
				RendererBasic::SetPolygonMode(RendererBasic::PolygonMode::LINE);
				RendererBasic::DrawIndexed(IndexCount, 0, BaseVertex, (void*)(sizeof(uint32_t) * BaseIndex));
			}
		}

		// glDrawElementsBaseVertex(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * BaseIndex), BaseVertex);

		shader->Unbind();
	}

	void SubmeshH2M::RenderOutline(RefH2M<MeshH2M> parentMesh, RefH2M<MoravaShader> shader, const glm::mat4& entityTransform, EntityH2M entity)
	{
		parentMesh->GetVertexBuffer()->Bind();
		parentMesh->GetPipeline()->Bind();
		parentMesh->GetIndexBuffer()->Bind();

		glm::vec3 translation, rotation, scale;
		Math::DecomposeTransform(entityTransform * Transform, translation, rotation, scale);
		// scale *= 1.2f;
		glm::mat4 outlineTransform = Math::CreateTransform(translation, glm::quat(rotation), scale);

		shader->Bind();
		shader->SetMat4("u_Transform", outlineTransform);

		for (size_t i = 0; i < parentMesh->GetBoneTransforms().size(); i++)
		{
			std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
			shader->SetMat4(uniformName, parentMesh->GetBoneTransforms()[i]);
		}

		shader->SetBool("u_Animated", parentMesh->IsAnimated());

		RendererBasic::DisableDepthTest();

		RendererBasic::DrawIndexed(IndexCount, 0, BaseVertex, (void*)(sizeof(uint32_t) * BaseIndex));
		// glDrawElementsBaseVertex(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * BaseIndex), BaseVertex);

		shader->Unbind();
	}

	void* MeshH2M::GetDescriptorSet()
	{
		return &s_DescriptorSet;
	}

}
