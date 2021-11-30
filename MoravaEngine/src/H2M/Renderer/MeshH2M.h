#pragma once

#include "H2M/Core/Math/AABBH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/Pipeline.h"
#include "H2M/Renderer/IndexBuffer.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/MaterialAsset.h"
#include "H2M/Scene/EntityH2M.h"

#include "Core/Log.h"
#include "EnvMap/EnvMapMaterial.h"
#include "Material/Material.h"
#include "Mesh/Mesh.h"
#include "Shader/MoravaShader.h"
#include "Texture/MoravaTexture.h"


#include <glm/glm.hpp>

#include <string>
#include <vector>

struct aiNode;
struct aiAnimation;
struct aiNodeAnim;
struct aiScene;

namespace Assimp {
	class Importer;
}

namespace H2M {

	struct VertexH2M
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;

		bool operator==(const VertexH2M& other) const
		{
			return Position == other.Position && Normal == other.Normal && Texcoord == other.Texcoord;
		}
	};

	struct AnimatedVertexH2M
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;

		uint32_t IDs[4] = { 0, 0, 0, 0 };
		float Weights[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

		void AddBoneData(uint32_t BoneID, float Weight)
		{
			for (size_t i = 0; i < 4; i++)
			{
				if (Weights[i] == 0.0)
				{
					IDs[i] = BoneID;
					Weights[i] = Weight;
					return;
				}
			}

			// TODO: Keep top weights
			// HZ_CORE_WARN("Vertex has more than four bones/weights affecting it, extra data will be discarded (BoneID={0}, Weight={1})", BoneID, Weight);
			Log::GetLogger()->warn("Vertex has more than four bones/weights affecting it, extra data will be discarded (BoneID={0}, Weight={1})", BoneID, Weight);
		}
	};

	static const int NumAttributesH2M = 5;

	struct IndexH2M
	{
		uint32_t V1, V2, V3;
	};

	static_assert(sizeof(IndexH2M) == 3 * sizeof(uint32_t));

	struct BoneInfoH2M
	{
		glm::mat4 BoneOffset;
		glm::mat4 FinalTransformation;
	};

	struct VertexBoneDataH2M
	{
		uint32_t IDs[4];
		float Weights[4];

		VertexBoneDataH2M()
		{
			memset(IDs, 0, sizeof(IDs));
			memset(Weights, 0, sizeof(Weights));
		};

		void AddBoneData(uint32_t BoneID, float Weight)
		{
			for (size_t i = 0; i < 4; i++)
			{
				if (Weights[i] == 0.0)
				{
					IDs[i] = BoneID;
					Weights[i] = Weight;
					return;
				}
			}
			
			// should never get here - more bones than we have space for
			Log::GetLogger()->error("Too many bones!");
		}
	};

	struct TriangleH2M
	{
		VertexH2M V0, V1, V2;

		TriangleH2M(const VertexH2M& v0, const VertexH2M& v1, const VertexH2M& v2)
			: V0(v0), V1(v1), V2(v2) {}
	};

	class MeshH2M;
	class EntityH2M;

	class SubmeshH2M : public Mesh
	{
	public:
		void Render(Ref<MeshH2M> parentMesh, Ref<MoravaShader> shader, const glm::mat4& entityTransform, uint32_t samplerSlot,
			const std::map<std::string, Ref<EnvMapMaterial>>& envMapMaterials, EntityH2M entity, bool wireframeEnabledScene = false, bool wireframeEnabledModel = false);
		void RenderOutline(Ref<MeshH2M> parentMesh, Ref<MoravaShader> shader, const glm::mat4& entityTransform, EntityH2M entity);

	public:
		uint32_t BaseVertex;
		uint32_t BaseIndex;
		uint32_t MaterialIndex;
		uint32_t IndexCount;
		uint32_t VertexCount;

		glm::mat4 Transform{ 1.0f };
		AABB BoundingBox;

		std::string NodeName, MeshName;
	};

	//
	// HazelMeshAssetLegacy is a representation of an actual asset file on disk
	// Meshes are created from MeshAssets
	//
	class HazelMeshAssetLegacy : public Asset
	{
	public:
		HazelMeshAssetLegacy(const std::string& filename);
		HazelMeshAssetLegacy(const std::vector<VertexH2M>& vertices, const std::vector<IndexH2M>& indices, const glm::mat4& transform);
		virtual ~HazelMeshAssetLegacy();

		// TODO: implement remaining methods
	};

	class MeshH2M : public Mesh
	{
	public:
		MeshH2M(const std::string& filename);
		MeshH2M(const std::string& filename, Ref<MoravaShader> shader, Ref<HazelMaterial> material, bool isAnimated);
		virtual ~MeshH2M() override;

		virtual void Create() override;
		virtual void OnUpdate(Timestep ts, bool debug = false) override;
		void OnImGuiRender(uint32_t id = 0, bool* p_open = (bool*)0);
		void DumpVertexBuffer();

		void Render(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, Ref<EnvMapMaterial>>& envMapMaterials);
		void RenderSubmeshes(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, Ref<EnvMapMaterial>>& envMapMaterials, EntityH2M entity);

		// Getters
		std::vector<Ref<SubmeshH2M>>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<Ref<SubmeshH2M>>& GetSubmeshes() const { return m_Submeshes; }

		const std::vector<VertexH2M>& GetVertices() const { return m_StaticVertices; }
		const std::vector<IndexH2M>& GetIndices() const { return m_Indices; }
		Ref<HazelShader> GetMeshShader() { return m_MeshShader; }
		Ref<HazelMaterial> GetMaterial() { return m_BaseMaterial; }
		std::vector<Ref<HazelMaterial>>& GetMaterials() { return m_Materials; }
		const std::vector<Ref<HazelMaterial>>& GetMaterials() const { return m_Materials; }
		const std::vector<Ref<Texture2DH2M>>& GetTextures() const { return m_Textures; }
		std::vector<Ref<Texture2DH2M>>& GetTextures() { return m_Textures; }
		const std::string& GetFilePath() const { return m_FilePath; }

		const std::vector<TriangleH2M> GetTriangleCache(uint32_t index) const;
		// const std::vector<TriangleH2M> GetTriangleCache(uint32_t index) const { return m_TriangleCache.at(index); }

		Ref<VertexBuffer> GetVertexBuffer() { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() { return m_IndexBuffer; }
		Ref<Pipeline> GetPipeline() { return m_Pipeline; }
		const VertexBufferLayout& GetVertexBufferLayout() const { return m_VertexBufferLayout; }

		/**** BEGIN this code should be removed from MeshH2M Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****/
		struct MaterialDescriptor
		{
			VulkanShader::ShaderMaterialDescriptorSet DescriptorSet;
			std::vector<VkWriteDescriptorSet> WriteDescriptors;
		};
		const MaterialDescriptor& GetDescriptorSet(uint32_t index) { return m_MaterialDescriptors[index]; }

		// VkDescriptorSet& GetDescriptorSet();
		void* GetDescriptorSet();

		void AddMaterialTextureWriteDescriptor(uint32_t index, const std::string& name, Ref<Texture2DH2M> texture);
		// void UpdateAllDescriptors();
		void UpdateAllDescriptorSets(); // Vulkan branch, february 2021
		static AssetType GetStaticType() { return AssetType::MeshAsset; }
		AssetType GetAssetType() const { return GetStaticType(); }

		/**** END this code should be removed from MeshH2M Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****/

		// std::vector<Ref<HazelMaterialInstance>> GetMaterials() { return m_Materials; }
		// const std::vector<Ref<HazelMaterialInstance>>& GetMaterials() const { return m_Materials; }

		bool& IsAnimated() { return m_IsAnimated; }
		const std::vector<glm::mat4>& GetBoneTransforms() { return m_BoneTransforms; }

		// Setters
		inline void SetBaseMaterial(Ref<HazelMaterial> baseMaterial) { m_BaseMaterial = baseMaterial; }
		inline void SetTimeMultiplier(float timeMultiplier) { m_TimeMultiplier = timeMultiplier; }

		void DeleteSubmesh(Ref<SubmeshH2M> submesh);
		void CloneSubmesh(Ref<SubmeshH2M> submesh);
		const AABB& GetBoundingBox() const { return m_BoundingBox; }

	private:
		void BoneTransform(float time);
		void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform);
		void TraverseNodes(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName);
		uint32_t FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
		glm::vec3 InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::quat InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::vec3 InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim);

		void ImGuiNodeHierarchy(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		void SetupDefaultBaseMaterial();
		Ref<Texture2DH2M> LoadBaseTexture();

	private:
		Ref<Pipeline> m_Pipeline;

		std::vector<Ref<SubmeshH2M>> m_Submeshes;

		std::unique_ptr<Assimp::Importer> m_Importer;

		glm::mat4 m_InverseTransform;

		uint32_t m_BoneCount = 0;
		std::vector<BoneInfoH2M> m_BoneInfo;

		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		VertexBufferLayout m_VertexBufferLayout;

		std::vector<VertexH2M> m_StaticVertices;
		std::vector<AnimatedVertexH2M> m_AnimatedVertices;
		std::vector<IndexH2M> m_Indices;
		std::unordered_map<std::string, uint32_t> m_BoneMapping;
		std::unordered_map<aiNode*, std::vector<uint32_t>> m_NodeMap;
		std::vector<glm::mat4> m_BoneTransforms;
		const aiScene* m_Scene;

		// Materials
		Ref<HazelMaterial> m_BaseMaterial;
		Ref<Texture2DH2M> m_BaseTexture;
		Ref<HazelShader> m_MeshShader;
		std::vector<Ref<Texture2DH2M>> m_Textures;
		std::vector<Ref<Texture2DH2M>> m_NormalMaps;
		std::vector<Ref<HazelMaterial>> m_Materials;
		// std::vector<Ref<HazelMaterialInstance>> m_Materials;

		std::unordered_map<uint32_t, std::vector<TriangleH2M>> m_TriangleCache;

		AABB m_BoundingBox;

		std::vector<MaterialDescriptor> m_MaterialDescriptors;

		// Animation
		bool m_IsAnimated = false;
		float m_AnimationTime = 0.0f;
		float m_WorldTime = 0.0f;
		float m_TimeMultiplier = 1.0f;
		bool m_AnimationPlaying = true;

		friend class Renderer;
		friend class SceneHierarchyPanel;

	};
}
