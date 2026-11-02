/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/Math/AABB_H2M.h"
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#endif
#include "H2M/Renderer/PipelineH2M.h"
#include "H2M/Renderer/IndexBufferH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include "Core/Log.h"
#include "EnvMap/EnvMapMaterial.h"
#include "Material/Material.h"
#include "Mesh/Mesh.h"
#include "Shader/MoravaShader.h"
#include "Texture/MoravaTexture.h"

#include "Animation/AnimationClip.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct aiNode;
struct aiAnimation;
struct aiNodeAnim;
struct aiScene;

namespace Assimp {
	class Importer;
}

namespace H2M
{

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

	struct AnimatedVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;

		uint32_t IDs[4] = { 0, 0,0, 0 };
		float Weights[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

		// The four largest weights are kept (the skinning shaders read four): false when a weight was left out, after which
		// NormalizeWeights makes the kept ones add up to 1 again
		bool AddBoneData(uint32_t BoneID, float Weight)
		{
			size_t smallest = 0;
			for (size_t i = 0; i < 4; i++)
			{
				if (Weights[i] == 0.0)
				{
					IDs[i] = BoneID;
					Weights[i] = Weight;
					return true;
				}
				if (Weights[i] < Weights[smallest])
				{
					smallest = i;
				}
			}
			if (Weight > Weights[smallest])
			{
				IDs[smallest] = BoneID;
				Weights[smallest] = Weight;
			}
			return false;
		}

		void NormalizeWeights()
		{
			const float sum = Weights[0] + Weights[1] + Weights[2] + Weights[3];
			if (sum > 0.0f)
			{
				for (float& weight : Weights)
				{
					weight /= sum;
				}
			}
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

	struct TriangleH2M
	{
		VertexH2M V0, V1, V2;

		TriangleH2M(const VertexH2M& v0, const VertexH2M& v1, const VertexH2M& v2)
			: V0(v0), V1(v1), V2(v2) {}
	};

	class ModelH2M;
	class EntityH2M;

	// A mesh: one part of a model, with a single material (one aiMesh of the model file). Its vertices and indices are a range
	// of the model's shared vertex and index buffers.
	class MeshH2M : public Mesh
	{
	public:
		void Render(RefH2M<ModelH2M> parentModel, RefH2M<MoravaShader> shader, const glm::mat4& entityTransform, uint32_t samplerSlot,
			const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials, EntityH2M entity, bool wireframeEnabledScene = false, bool wireframeEnabledModel = false);
		void RenderOutline(RefH2M<ModelH2M> parentModel, RefH2M<MoravaShader> shader, const glm::mat4& entityTransform, EntityH2M entity);

	public:
		uint32_t BaseVertex;
		uint32_t BaseIndex;
		uint32_t MaterialIndex;
		uint32_t IndexCount;
		uint32_t VertexCount = 0; // vertices of this mesh in the model's vertex buffer, from BaseVertex

		glm::mat4 Transform;
		AABB_H2M BoundingBox;

		std::string NodeName, MeshName;

		// The mesh has bone weights: its skinned vertices are already in model space (the bone matrices include the
		// node transforms), so it is drawn with the model transform only, not with Transform
		bool IsRigged = false;
	};

	// A model: a loaded model file (an aiScene), made of one or more meshes (GetMeshes), with its materials, skeleton and animations.
	class ModelH2M : public Mesh
	{
	public:
		ModelH2M(const std::string& filename);
		ModelH2M(const std::string& filename, RefH2M<MoravaShader> shader, RefH2M<MaterialH2M> material, bool isAnimated);
		virtual ~ModelH2M() override;

		virtual void Create() override;
		virtual void OnUpdate(TimestepH2M ts, bool debug = false) override;
		void OnImGuiRender(uint32_t id = 0, bool* p_open = (bool*)0);
		void DumpVertexBuffer();

		void Render(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials);
		void RenderMeshes(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, RefH2M<EnvMapMaterial>>& envMapMaterials, EntityH2M entity);

		// Getters
		std::vector<RefH2M<MeshH2M>>& GetMeshes() { return m_Meshes; }
		const std::vector<RefH2M<MeshH2M>>& GetMeshes() const { return m_Meshes; }

		RefH2M<ShaderH2M> GetMeshShader() { return m_MeshShader; }
		RefH2M<MaterialH2M> GetMaterial() { return m_BaseMaterial; }
		const std::vector<RefH2M<MaterialH2M>>& GetMaterials() const { return m_Materials; }
		std::vector<RefH2M<MaterialH2M>>& GetMaterials() { return m_Materials; }
		const std::vector<RefH2M<Texture2D_H2M>>& GetTextures() const { return m_Textures; }
		std::vector<RefH2M<Texture2D_H2M>>& GetTextures() { return m_Textures; }
		const std::string& GetFilePath() const { return m_FilePath; }

		const std::vector<TriangleH2M> GetTriangleCache(uint32_t index) const;

		RefH2M<VertexBufferH2M> GetVertexBuffer() { return m_VertexBuffer; }
		RefH2M<IndexBufferH2M> GetIndexBuffer() { return m_IndexBuffer; }
		RefH2M<PipelineH2M> GetPipeline() { return m_Pipeline; }
		const VertexBufferLayoutH2M& GetVertexBufferLayout() const { return m_VertexBufferLayout; }

		/**** BEGIN this code should be removed from ModelH2M Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****/
#if defined(GRAPHICS_API_VULKAN)
		struct MaterialDescriptor
		{
			VulkanShaderH2M::ShaderMaterialDescriptorSet DescriptorSet;
			std::vector<VkWriteDescriptorSet> WriteDescriptors;
			std::unordered_map<std::string, RefH2M<Texture2D_H2M>> Textures; // last texture written to each binding (e.g. "u_AlbedoTexture")
		};
		// Material descriptor sets (set 1 of the mesh shaders, VulkanShaderH2M::MaterialDescriptorSet): one per material, texture maps only
		const MaterialDescriptor& GetDescriptorSet(uint32_t index) { return m_MaterialDescriptors[index]; }
		// nullptr when the model has no material descriptor set for this index (e.g. a model without materials)
		const MaterialDescriptor* FindDescriptorSet(uint32_t index) const
		{
			if (index >= m_MaterialDescriptors.size() || m_MaterialDescriptors[index].DescriptorSet.DescriptorSets.empty()) return nullptr;
			return &m_MaterialDescriptors[index];
		}

#endif

		// VkDescriptorSet& GetDescriptorSet();
		void* GetDescriptorSet();

		// Per-object descriptor set (set 2, VulkanShaderH2M::ObjectDescriptorSet) of a skinned model: its bone matrices
		// (the BoneTransforms uniform buffer of this model's shader). VK_NULL_HANDLE for static models.
#if defined(GRAPHICS_API_VULKAN)
		VkDescriptorSet GetObjectDescriptorSet() const { return m_ObjectDescriptorSet.DescriptorSets.empty() ? VK_NULL_HANDLE : m_ObjectDescriptorSet.DescriptorSets[0]; }
#endif

		// Vulkan only (nothing happens in a build for another graphics API)
		void AddMaterialTextureWriteDescriptor(uint32_t index, const std::string& name, RefH2M<Texture2D_H2M> texture);
		// void UpdateAllDescriptors();
		void UpdateAllDescriptorSets(); // Vulkan branch, february 2021

		/**** END this code should be removed from ModelH2M Vulkan + OpenGL Living in Harmony // Hazel Live (25.02.2021) ****/

		// std::vector<RefH2M<MaterialInstanceH2M>> GetMaterials() { return m_Materials; }
		// const std::vector<RefH2M<MaterialInstanceH2M>>& GetMaterials() const { return m_Materials; }

		bool& IsAnimated() { return m_IsAnimated; }
		// True only if the loaded model file actually contains animations (IsAnimated() is a user-editable flag)
		bool HasAnimations() const;
		const std::vector<glm::mat4>& GetBoneTransforms() { return m_BoneTransforms; }

		// Animation playback (Models and Meshes panel in SceneEnvMapVulkan). Times are in animation ticks.
		// True if the vertex buffer has bone IDs and weights (the model was loaded with animations)
		bool IsSkinned() const { return !m_AnimatedVertices.empty(); }
		uint32_t GetBoneCount() const { return m_BoneCount; }
		// The bone matrices start with the inverse root node transform (see AnimationSkeleton): rigged meshes are drawn with
		// the model transform * this root transform (unit scale / axis conversion of the file)
		glm::mat4 GetRootTransform() const { return glm::inverse(m_InverseTransform); }
		uint32_t GetAnimationCount() const;
		std::string GetAnimationName(uint32_t index) const;
		uint32_t GetAnimationIndex() const { return m_AnimationIndex; }
		void SetAnimationIndex(uint32_t index); // also restarts the animation
		float GetAnimationDuration() const;
		float GetAnimationTicksPerSecond() const;
		float& AnimationTime() { return m_AnimationTime; }
		float& TimeMultiplier() { return m_TimeMultiplier; }
		bool& AnimationPlaying() { return m_AnimationPlaying; }

		// The skeleton and the file's own clips (Animation/AnimationClip.h): nullptr and empty for a model without animations
		const AnimationSkeleton* GetSkeleton() const { return m_Skeleton.get(); }
		const std::vector<AnimationClip>& GetClips() const { return m_Clips; }
		// A pose from outside (the animator, 6.4): the bone matrices are its until ReleasePose, and OnUpdate leaves them
		void SetPose(const AnimationPose& pose);
		void ReleasePose();
		bool IsPosedByAnimator() const { return m_PosedByAnimator; }

		// Setters
		inline void SetBaseMaterial(RefH2M<MaterialH2M> baseMaterial) { m_BaseMaterial = baseMaterial; }
		inline void SetTimeMultiplier(float timeMultiplier) { m_TimeMultiplier = timeMultiplier; }

		void DeleteMesh(RefH2M<MeshH2M> mesh);
		// Removes one part of the model (its geometry stays in the shared vertex/index buffers, just no longer drawn).
		// The triangle cache is keyed by mesh index, so the entries after it move down by one.
		void RemoveMesh(uint32_t index);
		void CloneMesh(RefH2M<MeshH2M> mesh);

	private:
		// The bone matrices of the played clip at time (in ticks; the bind pose when the model isn't animated)
		void BoneTransform(float time);
		void TraverseNodes(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		void ImGuiNodeHierarchy(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		void SetupDefaultBaseMaterial();
		RefH2M<Texture2D_H2M> LoadBaseTexture();

	private:
		RefH2M<PipelineH2M> m_Pipeline;
		RefH2M<VertexBufferH2M> m_VertexBuffer;
		RefH2M<IndexBufferH2M> m_IndexBuffer;
		VertexBufferLayoutH2M m_VertexBufferLayout;

		std::vector<glm::mat4> m_BoneTransforms;
		std::vector<RefH2M<MeshH2M>> m_Meshes;

		// Materials
		RefH2M<MoravaShader> m_MeshShader;

		std::unique_ptr<Assimp::Importer> m_Importer;

		glm::mat4 m_InverseTransform;

		uint32_t m_BoneCount = 0;
		std::vector<BoneInfo> m_BoneInfo;

		std::vector<VertexH2M> m_StaticVertices;
		std::vector<AnimatedVertex> m_AnimatedVertices;
		std::vector<Index> m_Indices;
		std::unordered_map<std::string, uint32_t> m_BoneMapping;

		// Materials
		RefH2M<MaterialH2M> m_BaseMaterial;
		RefH2M<Texture2D_H2M> m_BaseTexture;
		std::vector<RefH2M<Texture2D_H2M>> m_Textures;
		std::vector<RefH2M<Texture2D_H2M>> m_NormalMaps;
		std::vector<RefH2M<MaterialH2M>> m_Materials;
		// std::vector<RefH2M<MaterialInstanceH2M>> m_Materials;

		std::unordered_map<uint32_t, std::vector<TriangleH2M>> m_TriangleCache;

#if defined(GRAPHICS_API_VULKAN)
		std::vector<MaterialDescriptor> m_MaterialDescriptors;
		VulkanShaderH2M::ShaderMaterialDescriptorSet m_ObjectDescriptorSet; // skinned models: bone matrices (set 2)
#endif

		// Animation
		bool m_IsAnimated = false;
		float m_AnimationTime = 0.0f;
		float m_WorldTime = 0.0f;
		float m_TimeMultiplier = 1.0f;
		bool m_AnimationPlaying = true;
		uint32_t m_AnimationIndex = 0; // which of the file's animations is played
		std::shared_ptr<AnimationSkeleton> m_Skeleton;
		std::vector<AnimationClip> m_Clips;
		std::vector<AnimationBinding> m_ClipBindings; // m_Clips' on m_Skeleton
		AnimationPose m_Pose;                         // the played clip's, reused each frame
		bool m_PosedByAnimator = false;

		friend class RendererH2M;
		friend class SceneHierarchyPanelH2M;

	};
}
