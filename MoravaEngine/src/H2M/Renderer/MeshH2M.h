#pragma once

#include "H2M/Core/Math/AABB_H2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/PipelineH2M.h"
#include "H2M/Renderer/IndexBufferH2M.h"
#include "H2M/Renderer/TextureH2M.h"

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

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;

		bool operator==(const Vertex& other) const
		{
			return Position == other.Position && Normal == other.Normal && Texcoord == other.Texcoord;
		}
	};

	struct AnimatedVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;

		uint32_t IDs[4] = { 0, 0,0, 0 };
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
			Log::GetLogger()->warn("Vertex has more than four bones/weights affecting it, extra data will be discarded (BoneID={0}, Weight={1})", BoneID, Weight);
		}
	};

	static const int NumAttributes = 5;

	struct Index
	{
		uint32_t V1, V2, V3;
	};

	static_assert(sizeof(Index) == 3 * sizeof(uint32_t));

	struct BoneInfo
	{
		glm::mat4 BoneOffset;
		glm::mat4 FinalTransformation;
	};

	struct VertexBoneData
	{
		uint32_t IDs[4];
		float Weights[4];

		VertexBoneData()
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

	struct Triangle
	{
		Vertex V0, V1, V2;

		Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
			: V0(v0), V1(v1), V2(v2) {}
	};

	class MeshH2M;
	class EntityH2M;

	class SubmeshH2M : public Mesh
	{
	public:
		void Render(RefH2M<MeshH2M> parentMesh, RefH2M<MoravaShader> shader, const glm::mat4& entityTransform, uint32_t samplerSlot,
			const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials, EntityH2M entity, bool wireframeEnabledScene = false, bool wireframeEnabledModel = false);
		void RenderOutline(RefH2M<MeshH2M> parentMesh, RefH2M<MoravaShader> shader, const glm::mat4& entityTransform, EntityH2M entity);

	public:
		uint32_t BaseVertex;
		uint32_t BaseIndex;
		uint32_t MaterialIndex;
		uint32_t IndexCount;

		glm::mat4 Transform;
		AABB_H2M BoundingBox;

		std::string NodeName, MeshName;
	};

	class MeshH2M : public Mesh
	{
	public:
		MeshH2M(const std::string& filename);
		MeshH2M(const std::string& filename, RefH2M<MoravaShader> shader, RefH2M<MaterialH2M> material, bool isAnimated);
		virtual ~MeshH2M() override;

		virtual void Create() override;
		virtual void OnUpdate(TimestepH2M ts, bool debug = false) override;
		void OnImGuiRender(uint32_t id = 0, bool* p_open = (bool*)0);
		void DumpVertexBuffer();

		void Render(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials);
		void RenderSubmeshes(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials, EntityH2M entity);

		// Getters
		std::vector<SubmeshH2M>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<SubmeshH2M>& GetSubmeshes() const { return m_Submeshes; }

		RefH2M<ShaderH2M> GetMeshShader() { return m_MeshShader; }
		RefH2M<MaterialH2M> GetMaterial() { return m_BaseMaterial; }
		const std::vector<RefH2M<MaterialH2M>>& GetMaterials() const { return m_Materials; }
		std::vector<RefH2M<MaterialH2M>>& GetMaterials() { return m_Materials; }
		const std::vector<RefH2M<Texture2D_H2M>>& GetTextures() const { return m_Textures; }
		std::vector<RefH2M<Texture2D_H2M>>& GetTextures() { return m_Textures; }
		const std::string& GetFilePath() const { return m_FilePath; }

		const std::vector<Triangle> GetTriangleCache(uint32_t index) const;

		RefH2M<VertexBufferH2M> GetVertexBuffer() { return m_VertexBuffer; }
		RefH2M<IndexBufferH2M> GetIndexBuffer() { return m_IndexBuffer; }
		RefH2M<PipelineH2M> GetPipeline() { return m_Pipeline; }
		const VertexBufferLayoutH2M& GetVertexBufferLayout() const { return m_VertexBufferLayout; }

		/**** BEGIN this code should be removed from MeshH2M Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****/
		struct MaterialDescriptor
		{
			VulkanShaderH2M::ShaderMaterialDescriptorSet DescriptorSet;
			std::vector<VkWriteDescriptorSet> WriteDescriptors;
		};
		const MaterialDescriptor& GetDescriptorSet(uint32_t index) { return m_MaterialDescriptors[index]; }

		// VkDescriptorSet& GetDescriptorSet();
		void* GetDescriptorSet();

		void AddMaterialTextureWriteDescriptor(uint32_t index, const std::string& name, RefH2M<Texture2D_H2M> texture);
		// void UpdateAllDescriptors();
		void UpdateAllDescriptorSets(); // Vulkan branch, february 2021

		/**** END this code should be removed from MeshH2M Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****/

		// std::vector<RefH2M<HazelMaterialInstance>> GetMaterials() { return m_Materials; }
		// const std::vector<RefH2M<HazelMaterialInstance>>& GetMaterials() const { return m_Materials; }

		bool& IsAnimated() { return m_IsAnimated; }
		const std::vector<glm::mat4>& GetBoneTransforms() { return m_BoneTransforms; }

		// Setters
		inline void SetBaseMaterial(RefH2M<MaterialH2M> baseMaterial) { m_BaseMaterial = baseMaterial; }
		inline void SetTimeMultiplier(float timeMultiplier) { m_TimeMultiplier = timeMultiplier; }

		void DeleteSubmesh(SubmeshH2M submesh);
		void CloneSubmesh(SubmeshH2M submesh);

	private:
		void BoneTransform(float time);
		void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform);
		void TraverseNodes(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName);
		uint32_t FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
		glm::vec3 InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::quat InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::vec3 InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim);

		void ImGuiNodeHierarchy(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		void SetupDefaultBaseMaterial();
		RefH2M<Texture2D_H2M> LoadBaseTexture();

	private:
		RefH2M<PipelineH2M> m_Pipeline;
		RefH2M<VertexBufferH2M> m_VertexBuffer;
		RefH2M<IndexBufferH2M> m_IndexBuffer;
		VertexBufferLayoutH2M m_VertexBufferLayout;

		std::vector<glm::mat4> m_BoneTransforms;
		std::vector<SubmeshH2M> m_Submeshes;

		// Materials
		RefH2M<MoravaShader> m_MeshShader;

		std::unique_ptr<Assimp::Importer> m_Importer;

		glm::mat4 m_InverseTransform;

		uint32_t m_BoneCount = 0;
		std::vector<BoneInfo> m_BoneInfo;

		std::vector<Vertex> m_StaticVertices;
		std::vector<AnimatedVertex> m_AnimatedVertices;
		std::vector<Index> m_Indices;
		std::unordered_map<std::string, uint32_t> m_BoneMapping;

		// Materials
		RefH2M<MaterialH2M> m_BaseMaterial;
		RefH2M<Texture2D_H2M> m_BaseTexture;
		std::vector<RefH2M<Texture2D_H2M>> m_Textures;
		std::vector<RefH2M<Texture2D_H2M>> m_NormalMaps;
		std::vector<RefH2M<MaterialH2M>> m_Materials;
		// std::vector<RefH2M<HazelMaterialInstance>> m_Materials;

		std::unordered_map<uint32_t, std::vector<Triangle>> m_TriangleCache;

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
