#pragma once

#include "Hazel/Core/Math/AABB.h"
#include "Hazel/Renderer/Pipeline.h"
#include "Hazel/Renderer/IndexBuffer.h"
#include "Hazel/Renderer/HazelTexture.h"
// #include "Hazel/Core/Base.h"
// #include "Hazel/Renderer/VertexBuffer.h"
// #include "Hazel/Renderer/Pipeline.h"

#include "Core/Log.h"
#include "EnvMap/EnvMapMaterial.h"
#include "Material/Material.h"
#include "Mesh/Mesh.h"
#include "Shader/MoravaShader.h"
#include "Texture/Texture.h"

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

namespace Hazel {

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;
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

	class HazelMesh;
	class Entity;

	class Submesh : public Mesh
	{
	public:
		void Render(Ref<HazelMesh> parentMesh, Ref<MoravaShader> shader, const glm::mat4& entityTransform, uint32_t samplerSlot,
			const std::map<std::string, Ref<EnvMapMaterial>>& envMapMaterials, Entity entity);
		void RenderOutline(Ref<HazelMesh> parentMesh, Ref<MoravaShader> shader, const glm::mat4& entityTransform, Entity entity);

	public:
		uint32_t BaseVertex;
		uint32_t BaseIndex;
		uint32_t MaterialIndex;
		uint32_t IndexCount;

		glm::mat4 Transform;
		AABB BoundingBox;

		std::string NodeName, MeshName;
	};

	class HazelMesh : public Mesh
	{
	public:
		HazelMesh(const std::string& filename);
		HazelMesh(const std::string& filename, Ref<MoravaShader> shader, Ref<HazelMaterial> material, bool isAnimated);
		virtual ~HazelMesh() override;

		virtual void Create() override;
		virtual void OnUpdate(Timestep ts, bool debug) override;
		void OnImGuiRender(uint32_t id = 0, bool* p_open = (bool*)0);
		void DumpVertexBuffer();

		void Render(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, Ref<EnvMapMaterial>>& envMapMaterials);
		void RenderSubmeshes(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, Ref<EnvMapMaterial>>& envMapMaterials, Entity entity);

		// Getters
		std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }

		Ref<HazelShader> GetMeshShader() { return m_MeshShader; }
		Ref<HazelMaterial> GetMaterial() { return m_BaseMaterial; }
		// std::vector<Ref<HazelMaterialInstance>> GetMaterials() { return m_Materials; }
		// const std::vector<Ref<HazelMaterialInstance>>& GetMaterials() const { return m_Materials; }
		std::vector<Ref<HazelMaterial>> GetMaterials() { return m_Materials; }
		const std::vector<Ref<HazelMaterial>>& GetMaterials() const { return m_Materials; }
		inline std::vector<Ref<HazelTexture2D>>& GetTextures() { return m_Textures; }
		inline const std::vector<Ref<HazelTexture2D>>& GetTextures() const { return m_Textures; }
		inline bool& IsAnimated() { return m_IsAnimated; }
		inline const std::vector<glm::mat4>& GetBoneTransforms() { return m_BoneTransforms; }
		const std::vector<Triangle> GetTriangleCache(uint32_t index) const;

		Ref<VertexBuffer> GetVertexBuffer() { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() { return m_IndexBuffer; }
		Ref<Pipeline> GetPipeline() { return m_Pipeline; }

		// VkDescriptorSet& GetDescriptorSet();
		void* GetDescriptorSet();

		const VertexBufferLayout& GetVertexBufferLayout() const { return m_VertexBufferLayout; }

		// Setters
		inline void SetBaseMaterial(Ref<HazelMaterial> baseMaterial) { m_BaseMaterial = baseMaterial; }
		inline void SetTimeMultiplier(float timeMultiplier) { m_TimeMultiplier = timeMultiplier; }

		void DeleteSubmesh(Submesh submesh);
		void CloneSubmesh(Submesh submesh);


	private:
		void BoneTransform(float time);
		void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform);
		void TraverseNodes(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		void ImGuiNodeHierarchy(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName);
		uint32_t FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
		glm::vec3 InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::quat InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::vec3 InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim);
		void SetupDefaultBaseMaterial();
		Ref<HazelTexture2D> LoadBaseTexture();


	public:
		Ref<Pipeline> m_Pipeline;
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		VertexBufferLayout m_VertexBufferLayout;

		std::vector<glm::mat4> m_BoneTransforms;
		std::vector<Submesh> m_Submeshes;

		// Materials
		Ref<MoravaShader> m_MeshShader;

	private:
		std::unique_ptr<Assimp::Importer> m_Importer;

		glm::mat4 m_InverseTransform;

		uint32_t m_BoneCount = 0;
		std::vector<BoneInfo> m_BoneInfo;

		std::vector<Vertex> m_StaticVertices;
		std::vector<AnimatedVertex> m_AnimatedVertices;
		std::vector<Index> m_Indices;
		std::unordered_map<std::string, uint32_t> m_BoneMapping;

		// Materials
		Ref<HazelMaterial> m_BaseMaterial;
		Ref<HazelTexture2D> m_BaseTexture;
		std::vector<Ref<HazelTexture2D>> m_Textures;
		std::vector<Ref<HazelTexture2D>> m_NormalMaps;
		std::vector<Ref<HazelMaterial>> m_Materials;
		// std::vector<Ref<HazelMaterialInstance>> m_Materials;

		std::unordered_map<uint32_t, std::vector<Triangle>> m_TriangleCache;

		// std::vector<MaterialDescriptor> m_MaterialDescriptors;

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
