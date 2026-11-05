/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#include "ModelH2M.h"
#include "Assets/AssetFiles.h"
#include "Assets/ModelCook.h"
#include "Core/EnginePaths.h"

#include <algorithm>

// #include <GL/glew.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "H2M/Scene/EntityH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RendererH2M.h"

// TEMPORARY VULKAN INCLUDES
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"  // TODO: to be removed from ModelH2M
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h" // TODO: to be removed from ModelH2M
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"   // TODO: to be removed from ModelH2M
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"  // TODO: to be removed from ModelH2M
#endif

#include "Core/Log.h"
#include "Core/Math.h"
#include "Core/Util.h"
#include "Material/MaterialLibrary.h"
#include "Shader/MoravaShaderLibrary.h"

#include <filesystem>
#include <unordered_map>


namespace H2M
{

#define MESH_DEBUG_LOG 0 // 1: every step of a model's load, material by material (thousands of lines for a large scene)
#if MESH_DEBUG_LOG
	#define HZ_MESH_LOG(...) MORAVA_CORE_TRACE(__VA_ARGS__)
#else
	#define HZ_MESH_LOG(...)
#endif

#if defined(GRAPHICS_API_VULKAN)
	static VkDescriptorSet s_DescriptorSet;
	static std::vector<VkWriteDescriptorSet> s_WriteDescriptorSets;
#endif

	// glm::mat4 Mat4FromAssimpMat4(const aiMatrix4x4& matrix) moved to Math class

	// Loads a material map for the Vulkan-only maps below; returns an empty reference if the file is missing or fails to load
	static RefH2M<Texture2D_H2M> LoadMaterialMap(const std::string& texturePath, TextureCook::MapKind kind)
	{
		if (!AssetFiles::Exists(texturePath))
		{
			Log::GetLogger()->error("    Could not load texture: {0}", texturePath);
			return RefH2M<Texture2D_H2M>();
		}
		try
		{
			RefH2M<Texture2D_H2M> texture = ResourceManager::LoadMaterialMap(texturePath, kind);
			return texture && texture->Loaded() ? texture : RefH2M<Texture2D_H2M>();
		}
		catch (...)
		{
			Log::GetLogger()->error("    Could not load texture: {0}", texturePath);
			return RefH2M<Texture2D_H2M>();
		}
	}

	// Texture paths in a model file are relative to the model. Some models keep their maps in Textures/PBR/<model name>/
	// instead (e.g. Models/DamagedHelmet.gltf -> Textures/PBR/DamagedHelmet/), which is tried when the first path does not exist.
	static std::string ResolveTexturePath(const std::string& modelPath, const std::string& modelTexturePath)
	{
		// Models made on Windows may separate folders with '\', which is no separator elsewhere; '/' works everywhere
		std::string texturePath = modelTexturePath;
		std::replace(texturePath.begin(), texturePath.end(), '\\', '/');

		std::filesystem::path path = std::filesystem::path(modelPath).parent_path() / texturePath;
		if (AssetFiles::Exists(path.string()))
		{
			return path.string();
		}

		std::filesystem::path fallback = std::filesystem::path(TEXTURES_DIR "/PBR") / std::filesystem::path(modelPath).stem() / std::filesystem::path(texturePath).filename();
		if (AssetFiles::Exists(fallback.string()))
		{
			return fallback.string();
		}

		return path.string();
	}

	std::string ModelH2M::ResolveMapPath(const std::string& modelPath, const std::string& modelTexturePath)
	{
		return ResolveTexturePath(modelPath, modelTexturePath);
	}

	bool ModelH2M::IsModelFilePath(const std::string& path)
	{
		return GetAssetTypeOfFile(path) == AssetType::Model;
	}

	ModelH2M::ModelH2M(const std::string& filename)
	{
		m_FilePath = filename;
		m_BaseTexture = nullptr;
		m_MeshShader = nullptr;
		m_BaseMaterial = nullptr;
		m_IsAnimated = false;

		Create();
	}

	ModelH2M::ModelH2M(const std::string& filename, RefH2M<MoravaShader> shader, RefH2M<MaterialH2M> material, bool isAnimated)
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

	//	ModelH2M::ModelH2M(const std::string& filename)
	//		: m_FilePath(filename)
	//	{
	//		Create();
	//	}

	// Vulkan's mesh shader for a model file: a skinned model needs one for itself, the uniform buffer of its bone matrices
	// belongs to the shader (set 2). A static model has nothing of its own in it, so every static model shares the library's.
	static RefH2M<MoravaShader> CreateVulkanMeshShader(bool animated)
	{
		if (!animated)
		{
			return RefH2M<MoravaShader>(RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static"));
		}
		MoravaShaderSpecification specification;
		specification.ShaderType = MoravaShaderSpecification::ShaderType::HazelShader;
		specification.HazelShaderPath = RESOURCE_SHADERS_DIR "/HazelPBR_Anim.glsl";
		specification.ForceCompile = false; // the shader cache is recompiled when the source is newer
		return MoravaShader::Create(specification);
	}

	ModelH2M::ModelH2M(const RefH2M<ModelH2M>& source, const std::string& filename)
		: m_FileModel(source)
	{
		const ModelH2M& file = *m_FileModel.Raw();
		m_FilePath = filename;
		m_IsAnimated = file.m_IsAnimated;
		m_InverseTransform = file.m_InverseTransform;
		m_BoneCount = file.m_BoneCount;
		m_VertexBuffer = file.m_VertexBuffer;
		m_IndexBuffer = file.m_IndexBuffer;
		m_VertexBufferLayout = file.m_VertexBufferLayout;
		m_Skeleton = file.m_Skeleton;

		m_Meshes.reserve(file.m_Meshes.size());
		for (const RefH2M<MeshH2M>& sourceMesh : file.m_Meshes)
		{
			RefH2M<MeshH2M> mesh = RefH2M<MeshH2M>::Create();
			mesh->BaseVertex = sourceMesh->BaseVertex;
			mesh->BaseIndex = sourceMesh->BaseIndex;
			mesh->MaterialIndex = sourceMesh->MaterialIndex;
			mesh->IndexCount = sourceMesh->IndexCount;
			mesh->VertexCount = sourceMesh->VertexCount;
			mesh->SourceIndex = sourceMesh->SourceIndex;
			mesh->Source = sourceMesh;
			mesh->Transform = sourceMesh->Transform;
			mesh->BoundingBox = sourceMesh->BoundingBox;
			mesh->NodeName = sourceMesh->NodeName;
			mesh->MeshName = sourceMesh->MeshName;
			mesh->IsRigged = sourceMesh->IsRigged;
			m_Meshes.push_back(mesh);
		}

		m_MeshShader = file.m_MeshShader;
		if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan && m_IsAnimated)
		{
			m_MeshShader = CreateVulkanMeshShader(true);
			CreateObjectDescriptorSet();
		}
	}

	struct SharedModelFile
	{
		RefH2M<ModelH2M> Model;
		std::filesystem::file_time_type WriteTime;
	};

	static std::unordered_map<std::string, SharedModelFile> s_SharedModels; // by the file's canonical path

	std::vector<ModelH2M::LoadedFile> ModelH2M::GetLoadedFiles()
	{
		std::vector<LoadedFile> files;
		for (const auto& [path, shared] : s_SharedModels)
		{
			const ModelH2M& model = *shared.Model;
			LoadedFile file;
			file.Path = path;
			file.Users = shared.Model->GetRefCount() - 1; // not the registry's own reference
			file.Bytes = model.m_StaticVertices.size() * sizeof(VertexH2M) + model.m_AnimatedVertices.size() * sizeof(AnimatedVertex) + model.m_Indices.size() * sizeof(Index);
			files.push_back(std::move(file));
		}
		std::sort(files.begin(), files.end(), [](const LoadedFile& a, const LoadedFile& b) { return a.Path < b.Path; });
		return files;
	}

	RefH2M<ModelH2M> ModelH2M::LoadShared(const std::string& filepath)
	{
		std::error_code error;
		const std::filesystem::path file(filepath);
		const std::string key = std::filesystem::weakly_canonical(file, error).generic_string();
		const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(file, error);

		auto shared = s_SharedModels.find(key);
		if (shared == s_SharedModels.end() || shared->second.WriteTime != writeTime)
		{
			RefH2M<ModelH2M> model = RefH2M<ModelH2M>::Create(filepath);
			if (model->GetMeshes().empty())
			{
				return model;
			}
			// A file that changed replaces the entry: the models made before keep the old one until they go
			shared = s_SharedModels.insert_or_assign(key, SharedModelFile{ model, writeTime }).first;
		}
		return RefH2M<ModelH2M>::Create(shared->second.Model, filepath);
	}

	static int s_FramesUntilRelease = 0;

	void ModelH2M::ReleaseUnusedShared()
	{
		s_FramesUntilRelease = 8;
	}

	void ModelH2M::OnFrame()
	{
		if (s_FramesUntilRelease == 0 || --s_FramesUntilRelease > 0)
		{
			return;
		}
		size_t released = 0;
		for (auto it = s_SharedModels.begin(); it != s_SharedModels.end();)
		{
			if (it->second.Model->GetRefCount() == 1)
			{
				it = s_SharedModels.erase(it);
				released++;
			}
			else
			{
				++it;
			}
		}
		if (released > 0)
		{
			Log::GetLogger()->info("Models: {0} file(s) no scene uses any more were released, {1} kept", released, s_SharedModels.size());
		}
	}

	void ModelH2M::ReleaseAllShared()
	{
		s_SharedModels.clear();
	}

	void ModelH2M::Create()
	{
		HZ_MESH_LOG("H2M::ModelH2M: Loading model: {0}", m_FilePath.c_str());

		ModelData data;
		ModelCook::LoadInfo loadInfo;
		std::string loadError;
		if (!ModelCook::Load(m_FilePath, data, loadInfo, loadError) || data.Meshes.empty()) {
			Log::GetLogger()->error("Failed to load model file: {0}{1}{2}", m_FilePath, loadError.empty() ? "" : ": ", loadError);
			return;
		}

		// Animated models get the skinned vertex layout (bone IDs and weights) and the skinning shader, on OpenGL and Vulkan
		m_IsAnimated = data.IsAnimated;
		// m_MaterialInstance = std::make_shared<MaterialInstance>(m_BaseMaterial);

		if (!m_MeshShader)
		{
			/**** BEGIN MoravaShader the new API ****/
			if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
			{
				MoravaShaderSpecification moravaShaderSpecificationStatic;
				moravaShaderSpecificationStatic.ShaderType = MoravaShaderSpecification::ShaderType::MoravaShader;
				moravaShaderSpecificationStatic.VertexShaderPath = SHADERS_DIR "/Hazel/HazelPBR_Static.vs";
				moravaShaderSpecificationStatic.FragmentShaderPath = SHADERS_DIR "/Hazel/HazelPBR.fs";
				// moravaShaderSpecificationStatic.FragmentShaderPath = "Shaders/Hazel/HazelPBR_Static_CSM.fs"; // fragment shader with Cascaded Shadow Maps
				moravaShaderSpecificationStatic.ForceCompile = false;

				MoravaShaderSpecification moravaShaderSpecificationAnim;
				moravaShaderSpecificationAnim.ShaderType = MoravaShaderSpecification::ShaderType::MoravaShader;
				moravaShaderSpecificationAnim.VertexShaderPath = SHADERS_DIR "/Hazel/HazelPBR_Anim.vs";
				moravaShaderSpecificationAnim.FragmentShaderPath = SHADERS_DIR "/Hazel/HazelPBR.fs";
				moravaShaderSpecificationAnim.ForceCompile = false;

				m_MeshShader = m_IsAnimated ? MoravaShader::Create(moravaShaderSpecificationAnim) : MoravaShader::Create(moravaShaderSpecificationStatic);
			}
			else if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::Vulkan)
			{
				m_MeshShader = CreateVulkanMeshShader(m_IsAnimated);
			}
			else if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::DX11)
			{
				MoravaShaderSpecification moravaShaderSpecificationHazelDX11;
				moravaShaderSpecificationHazelDX11.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
				moravaShaderSpecificationHazelDX11.VertexShaderPath = SHADERS_DIR "/HLSL/BasicVertexShader.hlsl";
				moravaShaderSpecificationHazelDX11.PixelShaderPath = SHADERS_DIR "/HLSL/BasicPixelShader.hlsl";
				moravaShaderSpecificationHazelDX11.ForceCompile = false;
				m_MeshShader = ResourceManager::CreateOrLoadShader(moravaShaderSpecificationHazelDX11);
			}
			/**** END MoravaShader the new API ****/
		}

		m_InverseTransform = glm::inverse(data.Nodes[0].Matrix);

		m_StaticVertices = std::move(data.StaticVertices);
		m_AnimatedVertices = std::move(data.AnimatedVertices);
		m_Indices = std::move(data.Indices);

		HZ_MESH_LOG("H2M::ModelH2M: The model contains {0} meshes.", data.Meshes.size());

		m_Meshes.reserve(data.Meshes.size());
		size_t firstTriangle = 0;
		for (size_t m = 0; m < data.Meshes.size(); m++)
		{
			const ModelMeshData& sourceMesh = data.Meshes[m];

			RefH2M<MeshH2M> mesh = RefH2M<MeshH2M>::Create();
			mesh->BaseVertex = sourceMesh.BaseVertex;
			mesh->BaseIndex = sourceMesh.BaseIndex;
			mesh->MaterialIndex = sourceMesh.MaterialIndex;
			mesh->IndexCount = sourceMesh.IndexCount;
			mesh->VertexCount = sourceMesh.VertexCount;
			mesh->SourceIndex = (uint32_t)m;
			mesh->MeshName = sourceMesh.Name;
			mesh->IsRigged = sourceMesh.IsRigged;
			mesh->BoundingBox.Min = sourceMesh.BoundsMin;
			mesh->BoundingBox.Max = sourceMesh.BoundsMax;
			m_Meshes.push_back(mesh);

			// Triangle cache (animated models: the bind pose; mouse picking tests these triangles)
			std::vector<TriangleH2M>& triangles = m_TriangleCache[(uint32_t)m];
			triangles.reserve(sourceMesh.TriangleCount);
			for (size_t t = firstTriangle; t < firstTriangle + sourceMesh.TriangleCount; t++)
			{
				const Index& index = m_Indices[t];
				if (!m_IsAnimated)
				{
					if (index.V1 + mesh->BaseVertex < m_StaticVertices.size() &&
						index.V2 + mesh->BaseVertex < m_StaticVertices.size() &&
						index.V3 + mesh->BaseVertex < m_StaticVertices.size())
					{
						triangles.emplace_back(
							m_StaticVertices[index.V1 + mesh->BaseVertex],
							m_StaticVertices[index.V2 + mesh->BaseVertex],
							m_StaticVertices[index.V3 + mesh->BaseVertex]);
					}
				}
				else
				{
					if (index.V1 + mesh->BaseVertex < m_AnimatedVertices.size() &&
						index.V2 + mesh->BaseVertex < m_AnimatedVertices.size() &&
						index.V3 + mesh->BaseVertex < m_AnimatedVertices.size())
					{
						auto toStatic = [](const AnimatedVertex& v)
						{
							VertexH2M vertex;
							vertex.Position = v.Position;
							vertex.Normal = v.Normal;
							vertex.Tangent = v.Tangent;
							vertex.Binormal = v.Binormal;
							vertex.Texcoord = v.Texcoord;
							return vertex;
						};
						triangles.emplace_back(
							toStatic(m_AnimatedVertices[index.V1 + mesh->BaseVertex]),
							toStatic(m_AnimatedVertices[index.V2 + mesh->BaseVertex]),
							toStatic(m_AnimatedVertices[index.V3 + mesh->BaseVertex]));
					}
				}
			}
			firstTriangle += sourceMesh.TriangleCount;
		}

		ApplyNodeTransforms(data);

		// Display the list of all meshes
		for (size_t m = 0; m < m_Meshes.size(); m++) {
			HZ_MESH_LOG("-- Mesh ID {0} NodeName: '{1}'", m, m_Meshes[m]->NodeName);
		}

		PipelineSpecificationH2M pipelineSpecification;
		auto shader = m_MeshShader; // MoravaShader::Create("Resources/Shaders/VulkanWeekMesh.glsl", true);
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
			m_BoneCount = (uint32_t)data.BoneNames.size();
			m_BoneInfo.resize(m_BoneCount);
			for (uint32_t b = 0; b < m_BoneCount; b++)
			{
				m_BoneInfo[b].BoneOffset = data.BoneOffsets[b];
				m_BoneMapping[data.BoneNames[b]] = b;
			}

			// The Vulkan skinning shader (HazelPBR_Anim.glsl) has room for 128 bone matrices
			if (m_BoneCount > 128)
			{
				Log::GetLogger()->warn("ModelH2M: '{0}' has {1} bones; the Vulkan skinning shader supports 128, the others are not animated.", m_FilePath, m_BoneCount);
			}

			// The skeleton and the file's clips, sampled by BoneTransform and by the animator (Animation/AnimationClip.h)
			m_Skeleton = std::make_shared<AnimationSkeleton>(data.Nodes, m_BoneMapping, data.BoneOffsets);
			m_Clips = std::move(data.Clips);
			for (const AnimationClip& clip : m_Clips)
			{
				m_ClipBindings.push_back(AnimationBinding::Bind(*m_Skeleton, clip));
			}
		}

		/**** BEGIN Materials ****/

		// Materials
		if (!data.Materials.empty())
		{
			HZ_MESH_LOG("---- Materials - {0} ----", m_FilePath);

			m_MeshShader->Bind();

			m_Textures.resize(data.Materials.size());
			m_Materials.resize(data.Materials.size());
#if defined(GRAPHICS_API_VULKAN)
			m_MaterialDescriptors.resize(data.Materials.size()); // TODO: to be removed from ModelH2M
#endif

			RefH2M<Texture2D_H2M> whiteTexture = RendererH2M::GetWhiteTexture();

			for (uint32_t i = 0; i < (uint32_t)data.Materials.size(); i++)
			{
				const ModelMaterialData& sourceMaterial = data.Materials[i];

				auto mi = MaterialH2M::Create(m_MeshShader, sourceMaterial.Name);
				// auto mi = HazelMaterial::Create(m_BaseMaterial, aiMaterialName.data);
				// auto mi = RefH2M<MaterialInstanceH2M>::Create(m_BaseMaterial, aiMaterialName.data);
				m_Materials[i] = mi;

				/**** BEGIN to be removed from ModelH2M ****/
#if defined(GRAPHICS_API_VULKAN)
				if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
				{
					// HazelRenderer::Submit([instance, shader, i]() mutable {});
					{
						// The material set (set 1) holds only the texture maps (written below with AddMaterialTextureWriteDescriptor).
						// Camera and scene data are in the renderer's per-frame set (set 0), the bone matrices in the per-object set (set 2).
						MaterialDescriptor& materialDescriptor = m_MaterialDescriptors[i];
						materialDescriptor.DescriptorSet = shader.As<VulkanShaderH2M>()->CreateDescriptorSets(VulkanShaderH2M::MaterialDescriptorSet);
					}
				}
#endif
				/**** END to be removed from ModelH2M ****/

				HZ_MESH_LOG("  {0} (Index = {1})", sourceMaterial.Name, i);

				const glm::vec3 diffuseColor = sourceMaterial.DiffuseColor; // black when the file has none
				const glm::vec3 albedoColor = sourceMaterial.HasDiffuseColor ? diffuseColor : glm::vec3(0.8f);

				m_MeshShader->SetFloat3("u_MaterialUniforms.AlbedoColor", albedoColor);
				mi->Set("u_MaterialUniforms.AlbedoColor", albedoColor);

				const float shininess = sourceMaterial.HasShininess ? sourceMaterial.Shininess : 80.0f; // Default value
				float metalness = sourceMaterial.HasReflectivity ? sourceMaterial.Reflectivity : 0.0f;

				// float roughness = 1.0f - shininess * 0.01f;
				// roughness *= roughness;
				float roughness = 1.0f - glm::sqrt(shininess / 100.0f);
				HZ_MESH_LOG("    COLOR = {0}, {1}, {2}", diffuseColor.r, diffuseColor.g, diffuseColor.b);
				HZ_MESH_LOG("    ROUGHNESS = {0}", roughness);
				HZ_MESH_LOG("    METALNESS = {0}", metalness);

				// Material values used when there is no roughness/metalness map (Vulkan reads them from the material;
				// the m_MeshShader->SetFloat calls below only apply to OpenGL)
				if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
				{
					// PBR formats (glTF) store the values directly
					if (sourceMaterial.HasMetallicFactor) metalness = sourceMaterial.MetallicFactor;
					if (sourceMaterial.HasRoughnessFactor) roughness = sourceMaterial.RoughnessFactor;
				}
				mi->Set("u_MaterialUniforms.Roughness", roughness);
				mi->Set("u_MaterialUniforms.Metalness", metalness);

				// BEGIN the material data section
				RefH2M<MeshH2M> meshPtr = RefH2M<MeshH2M>();
				if (i < m_Meshes.size()) {
					meshPtr = m_Meshes[i];
				}

				RefH2M<MaterialData> materialData = MaterialLibrary::AddNewMaterial(m_Materials[i], meshPtr);
				// END the material data section

				bool hasAlbedoMap = !sourceMaterial.AlbedoMap.empty();
				bool fallback = !hasAlbedoMap;

				if (hasAlbedoMap)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::string texturePath = ResolveTexturePath(m_FilePath, sourceMaterial.AlbedoMap);
					HZ_MESH_LOG("    Albedo map path = '{0}'", texturePath);

					RefH2M<Texture2D_H2M> texture = RefH2M<Texture2D_H2M>();
					try {
						// texture = Texture2D_H2M::Create(texturePath, false);
						texture = ResourceManager::LoadMaterialMap(texturePath, TextureCook::MapKind::Color);
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
							AddMaterialTextureWriteDescriptor(i, "u_AlbedoTexture", texture); // TODO: to be removed from ModelH2M
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
						m_MeshShader->SetFloat3("u_MaterialUniforms.AlbedoColor", diffuseColor);
						mi->Set("u_MaterialUniforms.AlbedoColor", diffuseColor); // redundant

						HZ_MESH_LOG("Mesh has no Albedo map.");

						fallback = true;
					}
				}
				else
				{
					m_MeshShader->SetFloat3("u_MaterialUniforms.AlbedoColor", diffuseColor);
					mi->Set("u_MaterialUniforms.AlbedoColor", diffuseColor); // redundant
					HZ_MESH_LOG("    No albedo map");
				}

				if (fallback)
				{
					// HZ_MESH_LOG("    No albedo map");

					// Vulkan binds the texture to its sampler; OpenGL sets the sampler uniform to the texture unit (the Vulkan
					// material has no such uniform: setting it logged an error per mesh)
					if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
					{
						AddMaterialTextureWriteDescriptor(i, "u_AlbedoTexture", whiteTexture); // TODO: to be removed from ModelH2M
					}
					else
					{
						m_MeshShader->SetInt("u_AlbedoTexture", whiteTexture->GetID());
						mi->Set("u_AlbedoTexture", whiteTexture->GetID());
					}
				}

				// Normal maps
				m_MeshShader->SetFloat("u_MaterialUniforms.NormalTexToggle", 0.0f);
				mi->Set("u_MaterialUniforms.NormalTexToggle", 0.0f); // redundant

				bool hasNormalMap = !sourceMaterial.NormalMap.empty();
				fallback = !hasNormalMap;

				if (hasNormalMap)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::string texturePath = ResolveTexturePath(m_FilePath, sourceMaterial.NormalMap);
					HZ_MESH_LOG("    Normal map path = '{0}'", texturePath);

					RefH2M<Texture2D_H2M> texture = RefH2M<Texture2D_H2M>();
					try {
						// texture = Texture2D_H2M::Create(texturePath, false);
						texture = ResourceManager::LoadMaterialMap(texturePath, TextureCook::MapKind::Normal);
						m_Textures.push_back(texture);
					}
					catch (...) {
						Log::GetLogger()->warn("The NORMAL map failed to load. Loading the default texture placeholder instead.");
						texture = Texture2D_H2M::Create(TEXTURES_DIR "/normal_map_default.png", false);
					}

					if (texture && texture->Loaded())
					{
						m_Textures.push_back(texture);

						mi->Set("u_NormalTexture", texture);
						// mi->Set("u_MaterialUniforms.UseNormalMap", true);

						if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
						{
							AddMaterialTextureWriteDescriptor(i, "u_NormalTexture", texture); // TODO: to be removed from ModelH2M
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
					HZ_MESH_LOG("    No normal map");

					// Vulkan binds the texture to its sampler; OpenGL sets the sampler uniform to the texture unit (the Vulkan
					// material has no such uniform: setting it logged an error per mesh)
					if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
					{
						AddMaterialTextureWriteDescriptor(i, "u_NormalTexture", whiteTexture); // TODO: to be removed from ModelH2M
					}
					else
					{
						m_MeshShader->SetInt("u_NormalTexture", whiteTexture->GetID());
						mi->Set("u_NormalTexture", whiteTexture->GetID());
					}
				}

				// Roughness map
				// m_MeshShader->SetFloat("u_MaterialUniforms.Roughness", 1.0f);
				// m_MeshShader->SetFloat("u_MaterialUniforms.RoughnessTexToggle", 0.0f);

				std::string roughnessMap = sourceMaterial.ShininessMap;
				// PBR formats (glTF: the metallicRoughness map, roughness in G) - Vulkan only, the OpenGL shader reads roughness from R
				if (roughnessMap.empty() && RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
				{
					roughnessMap = sourceMaterial.DiffuseRoughnessMap;
				}
				const bool hasRoughnessMap = !roughnessMap.empty();
				fallback = !hasRoughnessMap;
				std::string roughnessTexturePath;

				if (hasRoughnessMap)
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::string texturePath = ResolveTexturePath(m_FilePath, roughnessMap);
					// HZ_MESH_LOG("    Roughness map path = '{0}'", texturePath);
					HZ_MESH_LOG("    Roughness map path = '{0}'", texturePath);
					roughnessTexturePath = texturePath;

					RefH2M<Texture2D_H2M> texture = RefH2M<Texture2D_H2M>();
					try {
						// texture = Texture2D_H2M::Create(texturePath, false);
						texture = ResourceManager::LoadMaterialMap(texturePath, TextureCook::MapKind::Data);
					}
					catch (...) {
						Log::GetLogger()->warn("The ROUGHNESS map failed to load. Loading the default texture placeholder instead.");
						texture = LoadBaseTexture();
					}

					if (texture && texture->Loaded()) // null when the file is missing or failed to load
					{
						m_Textures[i] = texture;
						mi->Set("u_RoughnessTexture", texture); // VulkanMaterial::FindResourceDeclaration - no resources found (name 'u_RoughnessTexture')!
						// mi->Set("u_MaterialUniforms.UseRoughnessMap", true); // VulkanMaterial::Set - Could not find uniform with name 'u_MaterialUniforms.UseRoughnessMap'!

						if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
						{
							AddMaterialTextureWriteDescriptor(i, "u_RoughnessTexture", texture); // TODO: to be removed from ModelH2M
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
					HZ_MESH_LOG("    No roughness map");

					m_MeshShader->SetFloat("u_MaterialUniforms.Roughness", roughness);

					// The shader samples u_RoughnessTexture: bind a placeholder like for the other maps
					// (an unwritten descriptor is invalid in Vulkan)
					if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
					{
						AddMaterialTextureWriteDescriptor(i, "u_RoughnessTexture", whiteTexture);
					}
				}

				bool metalnessTextureFound = false;
				if (!sourceMaterial.ReflectionFactorMap.empty())
				{
					// TODO: Temp - this should be handled by Hazel's filesystem
					std::string texturePath = ResolveTexturePath(m_FilePath, sourceMaterial.ReflectionFactorMap);
					HZ_MESH_LOG("    Metalness map path = '{0}'", texturePath);

					RefH2M<Texture2D_H2M> texture = RefH2M<Texture2D_H2M>();
					try {
						// texture = Texture2D_H2M::Create(texturePath, false);
						texture = ResourceManager::LoadMaterialMap(texturePath, TextureCook::MapKind::Data);
					}
					catch (...) {
						Log::GetLogger()->warn("The METALNESS map failed to load. Loading the default texture placeholder instead.");
						texture = LoadBaseTexture();
					}

					if (texture && texture->Loaded()) // null when the file is missing or failed to load
					{
						metalnessTextureFound = true;

						m_Textures.push_back(texture);
						mi->Set("u_MetalnessTexture", texture); // VulkanMaterial::FindResourceDeclaration - no resources found (name 'u_MetalnessTexture')!
						// mi->Set("u_MaterialUniforms.UseMetalnessMap", true); // VulkanMaterial::Set - Could not find uniform with name 'u_MaterialUniforms.UseMetalnessMap'!

						if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
						{
							AddMaterialTextureWriteDescriptor(i, "u_MetalnessTexture", texture); // TODO: to be removed from ModelH2M
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
				}

				// PBR formats (glTF: the metallicRoughness map, metalness in B) - Vulkan only, the OpenGL shader reads metalness from R
				std::string metalnessTexturePath;
				if (!metalnessTextureFound && RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan &&
					!sourceMaterial.MetalnessMap.empty())
				{
					metalnessTexturePath = ResolveTexturePath(m_FilePath, sourceMaterial.MetalnessMap);
					HZ_MESH_LOG("    Metalness map path = '{0}'", metalnessTexturePath);

					RefH2M<Texture2D_H2M> texture = LoadMaterialMap(metalnessTexturePath, TextureCook::MapKind::Data);
					if (texture)
					{
						metalnessTextureFound = true;
						m_Textures.push_back(texture);
						mi->Set("u_MetalnessTexture", texture);
						AddMaterialTextureWriteDescriptor(i, "u_MetalnessTexture", texture); // TODO: to be removed from ModelH2M
						mi->Set("u_MaterialUniforms.MetalnessTexToggle", 1.0f);
						MaterialLibrary::AddTextureToEnvMapMaterial(MaterialTextureType::Metalness, metalnessTexturePath, materialData->EnvMapMaterialRef);
					}
				}

				if (!metalnessTextureFound)
				{
					HZ_MESH_LOG("    No metalness map");
					// Log::GetLogger()->error("    Could not load texture: {0}", texturePath);

					m_MeshShader->SetFloat("u_MaterialUniforms.Metalness", metalness);
					m_MeshShader->SetFloat("u_MaterialUniforms.MetalnessTexToggle", 0.0f);
				}

				fallback = !metalnessTextureFound;
				if (fallback)
				{
					HZ_MESH_LOG("    No metalness map");

					// Vulkan binds the texture to its sampler; OpenGL sets the sampler uniform to the texture unit (the Vulkan
					// material has no such uniform: setting it logged an error per mesh)
					if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
					{
						AddMaterialTextureWriteDescriptor(i, "u_MetalnessTexture", whiteTexture); // TODO: to be removed from ModelH2M
					}
					else
					{
						m_MeshShader->SetInt("u_MetalnessTexture", whiteTexture->GetID());
						mi->Set("u_MetalnessTexture", whiteTexture->GetID());
					}
				}

				// Emissive, ambient occlusion, tiling (Vulkan PBR shader, HazelPBR_Static.glsl)
				if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
				{
					mi->Set("u_MaterialUniforms.TilingFactor", 1.0f);

					// glTF packs metalness (B) and roughness (G) into one map; grayscale maps read the same in any channel
					bool packed = !roughnessTexturePath.empty() && roughnessTexturePath == metalnessTexturePath;
					mi->Set("u_MaterialUniforms.MetalRoughPacked", packed ? 1.0f : 0.0f);

					// Emissive map: color data (sRGB)
					RefH2M<Texture2D_H2M> emissiveTexture;
					if (!sourceMaterial.EmissiveMap.empty())
					{
						std::string texturePath = ResolveTexturePath(m_FilePath, sourceMaterial.EmissiveMap);
						HZ_MESH_LOG("    Emissive map path = '{0}'", texturePath);
						emissiveTexture = LoadMaterialMap(texturePath, TextureCook::MapKind::Color);
					}
					if (emissiveTexture)
					{
						// glTF: the map is multiplied by emissiveFactor
						float intensity = 1.0f;
						if (sourceMaterial.HasEmissiveColor)
						{
							const glm::vec3& emissiveColor = sourceMaterial.EmissiveColor;
							intensity = glm::max(emissiveColor.r, glm::max(emissiveColor.g, emissiveColor.b));
						}
						m_Textures.push_back(emissiveTexture);
						AddMaterialTextureWriteDescriptor(i, "u_EmissiveTexture", emissiveTexture);
						mi->Set("u_MaterialUniforms.EmissiveTexToggle", 1.0f);
						mi->Set("u_MaterialUniforms.EmissiveIntensity", intensity > 0.0f ? intensity : 1.0f);
					}
					else
					{
						AddMaterialTextureWriteDescriptor(i, "u_EmissiveTexture", whiteTexture); // placeholder, the toggle is off
						mi->Set("u_MaterialUniforms.EmissiveTexToggle", 0.0f);
						mi->Set("u_MaterialUniforms.EmissiveIntensity", 1.0f);
					}

					// Ambient occlusion map: linear data. glTF's occlusionTexture arrives as LIGHTMAP
					RefH2M<Texture2D_H2M> aoTexture;
					if (!sourceMaterial.AmbientOcclusionMap.empty())
					{
						std::string texturePath = ResolveTexturePath(m_FilePath, sourceMaterial.AmbientOcclusionMap);
						HZ_MESH_LOG("    AO map path = '{0}'", texturePath);
						aoTexture = LoadMaterialMap(texturePath, TextureCook::MapKind::Data);
					}
					if (aoTexture)
					{
						m_Textures.push_back(aoTexture);
						AddMaterialTextureWriteDescriptor(i, "u_AOTexture", aoTexture);
						mi->Set("u_MaterialUniforms.AOTexToggle", 1.0f);
					}
					else
					{
						AddMaterialTextureWriteDescriptor(i, "u_AOTexture", whiteTexture); // placeholder, the toggle is off
						mi->Set("u_MaterialUniforms.AOTexToggle", 0.0f);
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

		CreateObjectDescriptorSet();

		HZ_MESH_LOG("H2M::ModelH2M: Creating a Vertex Buffer...");

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
			m_VertexBuffer = VertexBufferH2M::Create(m_StaticVertices.data(), (uint32_t)sizeof(VertexH2M), (uint32_t)m_StaticVertices.size());

			m_VertexBufferLayout = {
							{ ShaderDataTypeH2M::Float3, "a_Position" },
							{ ShaderDataTypeH2M::Float3, "a_Normal" },
							{ ShaderDataTypeH2M::Float3, "a_Tangent" },
							{ ShaderDataTypeH2M::Float3, "a_Binormal" },
							{ ShaderDataTypeH2M::Float2, "a_TexCoord" },
			};
		}

		HZ_MESH_LOG("H2M::ModelH2M: Creating an Index Buffer...");
		m_IndexBuffer = IndexBufferH2M::Create(m_Indices.data(), (uint32_t)m_Indices.size() * sizeof(Index));

		/**** BEGIN Create pipeline ****/
		// Only for OpenGL. In Vulkan the renderer's pipelines draw every model
		if (RendererAPI_H2M::Current() != RendererAPITypeH2M::Vulkan)
		{

			pipelineSpecification.Layout = m_VertexBufferLayout;

			FramebufferSpecificationH2M framebufferSpec;
			framebufferSpec.Attachments = { ImageFormatH2M::RGBA32F, ImageFormatH2M::Depth };
			// Placeholder render target (never drawn into); a 0x0 size makes glTextureStorage2D fail with GL_INVALID_VALUE
			framebufferSpec.Width = 1;
			framebufferSpec.Height = 1;
			framebufferSpec.Samples = 1;
			framebufferSpec.ClearOnLoad = false;
			framebufferSpec.ClearColor = { 0.1f, 0.5f, 0.5f, 1.0f };
			framebufferSpec.DebugName = "ModelH2M Framebuffer";

			RefH2M<FramebufferH2M> framebuffer = FramebufferH2M::Create(framebufferSpec);

			RenderPassSpecificationH2M renderPassSpec = {};
			renderPassSpec.TargetFramebuffer = framebuffer;
			renderPassSpec.DebugName = "ModelH2M";

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
		m_Nodes = std::move(data.Nodes);
		m_NodeMeshes = std::move(data.NodeMeshes);

		Log::GetLogger()->info("Model '{0}': {1} meshes, {2} vertices, {3} triangles ({4} in {5:.1f} ms)", m_FilePath, m_Meshes.size(), totalVertices, m_Indices.size(),
			loadInfo.FromCache ? "cooked file read" : "imported and cooked", loadInfo.Milliseconds);

		HZ_MESH_LOG("H2M::ModelH2M: Total vertices: {0}", totalVertices);
		HZ_MESH_LOG("H2M::ModelH2M: Total indices: {0}", m_Indices.size());
	}

	ModelH2M::~ModelH2M()
	{
	}

	void ModelH2M::CreateObjectDescriptorSet()
	{
		// Per-object descriptor set of a skinned model (set 2): the bone matrices, updated every frame by the renderer
#if defined(GRAPHICS_API_VULKAN)
		if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan && m_IsAnimated)
		{
			RefH2M<VulkanShaderH2M> vulkanShader = m_MeshShader.As<VulkanShaderH2M>();
			if (vulkanShader->HasDescriptorSet(VulkanShaderH2M::ObjectDescriptorSet))
			{
				m_ObjectDescriptorSet = vulkanShader->CreateDescriptorSets(VulkanShaderH2M::ObjectDescriptorSet);

				VkWriteDescriptorSet writeDescriptorSet = *vulkanShader->GetDescriptorSet("BoneTransforms", VulkanShaderH2M::ObjectDescriptorSet);
				writeDescriptorSet.dstSet = m_ObjectDescriptorSet.DescriptorSets[0];
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.pBufferInfo = &vulkanShader->GetUniformBuffer(0, VulkanShaderH2M::ObjectDescriptorSet).Descriptor;
				vkUpdateDescriptorSets(VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &writeDescriptorSet, 0, nullptr);
			}
		}
#endif
	}

	void ModelH2M::AddMaterialTextureWriteDescriptor(uint32_t index, const std::string& name, RefH2M<Texture2D_H2M> texture)
	{
#if defined(GRAPHICS_API_VULKAN)
		// RefH2M<ModelH2M> instance = this;
		// HazelRenderer::Submit([instance, index, name, texture]() mutable {});
		{
			MaterialDescriptor& materialDescriptor = m_MaterialDescriptors[index];

			RefH2M<ShaderH2M> shader = m_Materials[index]->GetShader();
			const VkWriteDescriptorSet* wds = shader.As<VulkanShaderH2M>()->GetDescriptorSet(name, VulkanShaderH2M::MaterialDescriptorSet);
			H2M_CORE_ASSERT(wds);

			VkWriteDescriptorSet descriptorSet = *wds;
			descriptorSet.dstSet = *materialDescriptor.DescriptorSet.DescriptorSets.data();
			descriptorSet.descriptorCount = (uint32_t)materialDescriptor.DescriptorSet.DescriptorSets.size();
			auto& imageInfo = texture.As<VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
			descriptorSet.pImageInfo = &imageInfo;
			materialDescriptor.WriteDescriptors.push_back(descriptorSet);
			materialDescriptor.Textures[name] = texture;
		}
#endif
	}

	void ModelH2M::UpdateAllDescriptorSets()
	{
#if defined(GRAPHICS_API_VULKAN)
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
#endif
	}

	/**** BEGIN removed in Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****
	void ModelH2M::UpdateAllDescriptors()
	{
		// RefH2M<ModelH2M> instance = this;
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
	 **** END removed in Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****/

	bool ModelH2M::HasAnimations() const
	{
		return !GetClips().empty();
	}

	uint32_t ModelH2M::GetAnimationCount() const
	{
		return (uint32_t)GetClips().size();
	}

	std::string ModelH2M::GetAnimationName(uint32_t index) const
	{
		if (index >= GetAnimationCount())
		{
			return std::string();
		}
		return GetClips()[index].GetName();
	}

	void ModelH2M::SetAnimationIndex(uint32_t index)
	{
		if (index < GetAnimationCount())
		{
			m_AnimationIndex = index;
			m_AnimationTime = 0.0f;
		}
	}

	float ModelH2M::GetAnimationDuration() const
	{
		return m_AnimationIndex < GetAnimationCount() ? GetClips()[m_AnimationIndex].GetDuration() * GetClips()[m_AnimationIndex].GetTicksPerSecond() : 0.0f;
	}

	float ModelH2M::GetAnimationTicksPerSecond() const
	{
		if (m_AnimationIndex >= GetAnimationCount())
		{
			return 25.0f;
		}
		return GetClips()[m_AnimationIndex].GetTicksPerSecond();
	}

	void ModelH2M::OnUpdate(TimestepH2M ts, bool debug)
	{
		if (m_PosedByAnimator)
		{
			return; // the animator's pose (SetPose)
		}
		// m_IsAnimated can be switched on in the UI; only animate models that actually have animations
		if (m_IsAnimated && HasAnimations())
		{
			if (m_AnimationIndex >= GetAnimationCount())
			{
				m_AnimationIndex = 0;
			}

			if (m_AnimationPlaying)
			{
				m_WorldTime += ts;

				float ticksPerSecond = GetAnimationTicksPerSecond() * m_TimeMultiplier;
				m_AnimationTime += ts * ticksPerSecond;
				const float duration = GetAnimationDuration();
				if (duration > 0.0f)
				{
					m_AnimationTime = fmod(m_AnimationTime, duration);
				}
			}

			// TODO: We only need to recalc bones if rendering has been requested at the current animation frame
			BoneTransform(m_AnimationTime);
		}
		else if (IsSkinned())
		{
			// Animation switched off: the skinned vertices still need bone matrices, those of the bind pose
			BoneTransform(0.0f);
		}
	}

	void ModelH2M::ApplyNodeTransforms(const ModelData& data)
	{
		std::vector<glm::mat4> world(data.Nodes.size());
		for (size_t n = 0; n < data.Nodes.size(); n++)
		{
			const AnimationSkeleton::SourceNode& node = data.Nodes[n];
			world[n] = node.Parent >= 0 ? world[node.Parent] * node.Matrix : node.Matrix;
			for (uint32_t meshIndex : data.NodeMeshes[n])
			{
				m_Meshes[meshIndex]->NodeName = node.Name;
				m_Meshes[meshIndex]->Transform = world[n];
			}
		}
	}

	void ModelH2M::SetupDefaultBaseMaterial()
	{
		// Setup default Material
		TextureInfo textureInfoDefault = {};
		textureInfoDefault.albedo    = TEXTURES_DIR "/plain.png";
		textureInfoDefault.normal    = TEXTURES_DIR "/normal_map_default.png";
		textureInfoDefault.metallic  = TEXTURES_DIR "/plain.png";
		textureInfoDefault.roughness = TEXTURES_DIR "/plain.png";
		textureInfoDefault.emissive  = TEXTURES_DIR "/plain.png";
		textureInfoDefault.ao        = TEXTURES_DIR "/plain.png";
		m_BaseMaterial = RefH2M<Material>::Create(textureInfoDefault, 0.0f, 0.0f);
	}

	RefH2M<Texture2D_H2M> ModelH2M::LoadBaseTexture()
	{
		if (!m_BaseTexture) {
			try {
				m_BaseTexture = Texture2D_H2M::Create(TEXTURES_DIR "/plain.png", false);
			}
			catch (...) {
				Log::GetLogger()->warn("Failed to load the base texture!");
				m_BaseTexture = nullptr;
			}
		}

		return m_BaseTexture;
	}

	void ModelH2M::DeleteMesh(RefH2M<MeshH2M> mesh)
	{
		for (auto iterator = m_Meshes.cbegin(); iterator != m_Meshes.cend();)
		{
			if (iterator->Raw()->MeshName == mesh->MeshName)
			{
				iterator = m_Meshes.erase(iterator++);
				Log::GetLogger()->debug("ModelH2M::DeleteMesh erase '{0}'", mesh->MeshName);
			}
			else
			{
				++iterator;
			}
		}
	}

	void ModelH2M::RemoveMesh(uint32_t index)
	{
		if (index >= m_Meshes.size())
		{
			return;
		}
		m_Meshes.erase(m_Meshes.begin() + index);
	}

	void ModelH2M::CloneMesh(RefH2M<MeshH2M> mesh)
	{
		// Ref<MeshH2M> submeshCopy = Ref<MeshH2M>::Create(submesh);
		RefH2M<MeshH2M> meshCopy = RefH2M<MeshH2M>::Create();
		std::string appendix = Util::randomString(2);
		meshCopy->MeshName += "." + appendix;
		meshCopy->NodeName += "." + appendix;
		m_Meshes.push_back(meshCopy);
	}

	void ModelH2M::BoneTransform(float time)
	{
		if (!m_Skeleton)
		{
			return;
		}
		m_Skeleton->GetBindPose(m_Pose);
		if (m_IsAnimated && m_AnimationIndex < GetClips().size())
		{
			const AnimationClip& clip = GetClips()[m_AnimationIndex];
			AnimationSampling::SampleClip(clip, GetFileModel().m_ClipBindings[m_AnimationIndex], time / GetAnimationTicksPerSecond(), true, false, m_Pose);
		}
		m_Skeleton->ComputeBoneTransforms(m_Pose, m_BoneTransforms);
	}

	void ModelH2M::SetPose(const AnimationPose& pose)
	{
		if (m_Skeleton)
		{
			m_Skeleton->ComputeBoneTransforms(pose, m_BoneTransforms);
			m_PosedByAnimator = true;
		}
	}

	void ModelH2M::ReleasePose()
	{
		m_PosedByAnimator = false;
	}


	void ModelH2M::DumpVertexBuffer()
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

	const std::vector<TriangleH2M>& ModelH2M::GetTriangleCache(uint32_t index) const
	{
		static const std::vector<TriangleH2M> s_NoTriangles;
		if (index >= m_Meshes.size())
		{
			return s_NoTriangles;
		}
		const std::unordered_map<uint32_t, std::vector<TriangleH2M>>& cache = GetFileModel().m_TriangleCache;
		auto entry = cache.find(m_Meshes[index]->SourceIndex);
		return entry != cache.end() ? entry->second : s_NoTriangles;
	}

	void ModelH2M::Render(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials)
	{
		RefH2M<EnvMapMaterial> envMapMaterial = RefH2M<EnvMapMaterial>();

		m_VertexBuffer->Bind();
		m_Pipeline->Bind();
		m_IndexBuffer->Bind();

		for (RefH2M<MeshH2M> mesh : m_Meshes)
		{
			m_MeshShader->Bind();

			for (size_t i = 0; i < m_BoneTransforms.size(); i++)
			{
				std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");

				m_MeshShader->SetMat4(uniformName, m_BoneTransforms[i]);
			}

			m_MeshShader->SetMat4("u_Transform", transform * mesh->Transform);

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

			RefH2M<ModelH2M> instance = this;
			std::string materialUUID = MaterialLibrary::GetSubmeshMaterialUUID(instance, mesh, EntityH2M{});

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
				material = m_Materials[mesh->MaterialIndex];
				if (material && material->GetFlag(MaterialFlagH2M::DepthTest))
				{
					RendererBasic::EnableDepthTest();
				}
			}
			else
			{
				RendererBasic::DisableDepthTest();
			}

			RendererBasic::DrawIndexed(mesh->IndexCount, 0, mesh->BaseVertex, (void*)(sizeof(uint32_t) * mesh->BaseIndex));
			// glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);
		}
	}

	void ModelH2M::RenderMeshes(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials, EntityH2M entity)
	{
		for (RefH2M<MeshH2M> mesh : m_Meshes)
		{
			mesh->Render(this, m_MeshShader, transform, samplerSlot, envMapMaterials, entity);
		}
	}

	void MeshH2M::Render(RefH2M<ModelH2M> parentModel, RefH2M<MoravaShader> shader, const glm::mat4& entityTransform, uint32_t samplerSlot,
		const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials, EntityH2M entity, bool wireframeEnabledScene, bool wireframeEnabledModel)
	{
		RefH2M<EnvMapMaterial> envMapMaterial = RefH2M<EnvMapMaterial>();

		parentModel->GetVertexBuffer()->Bind();
		parentModel->GetPipeline()->Bind();
		parentModel->GetIndexBuffer()->Bind();

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

		std::string materialUUID = MaterialLibrary::GetSubmeshMaterialUUID(parentModel, this, entity);

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

		auto material = parentModel->GetMaterials()[MaterialIndex];
		if (material->GetFlag(MaterialFlagH2M::DepthTest))
		{
			RendererBasic::EnableDepthTest();
		}
		else
		{
			RendererBasic::DisableDepthTest();
		}

		shader->Bind();

		for (size_t i = 0; i < parentModel->GetBoneTransforms().size(); i++)
		{
			std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
			shader->SetMat4(uniformName, parentModel->GetBoneTransforms()[i]);
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

	void MeshH2M::RenderOutline(RefH2M<ModelH2M> parentModel, RefH2M<MoravaShader> shader, const glm::mat4& entityTransform, EntityH2M entity)
	{
		parentModel->GetVertexBuffer()->Bind();
		parentModel->GetPipeline()->Bind();
		parentModel->GetIndexBuffer()->Bind();

		glm::vec3 translation, rotation, scale;
		Math::DecomposeTransform(entityTransform * Transform, translation, rotation, scale);
		// scale *= 1.2f;
		glm::mat4 outlineTransform = Math::CreateTransform(translation, glm::quat(rotation), scale);

		shader->Bind();
		shader->SetMat4("u_Transform", outlineTransform);

		for (size_t i = 0; i < parentModel->GetBoneTransforms().size(); i++)
		{
			std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
			shader->SetMat4(uniformName, parentModel->GetBoneTransforms()[i]);
		}

		shader->SetBool("u_Animated", parentModel->IsAnimated());

		RendererBasic::DisableDepthTest();

		RendererBasic::DrawIndexed(IndexCount, 0, BaseVertex, (void*)(sizeof(uint32_t) * BaseIndex));
		// glDrawElementsBaseVertex(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * BaseIndex), BaseVertex);

		shader->Unbind();
	}

	void* ModelH2M::GetDescriptorSet()
	{
#if defined(GRAPHICS_API_VULKAN)
		return &s_DescriptorSet;
#else
		return nullptr;
#endif
	}

}
