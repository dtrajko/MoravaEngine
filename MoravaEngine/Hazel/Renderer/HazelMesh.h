#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../Core/Base.h"
#include "../Core/Math/AABB.h"
#include "../Platform/OpenGL/OpenGLBuffer.h"
#include "../Platform/OpenGL/OpenGLVertexArray.h"

#include "../../Log.h"
#include "../../Shader.h"
#include "../../Texture.h"
#include "../../Material.h"
#include "../../EnvMapMaterial.h"
#include "../../Mesh.h"

#include <string>

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

	class Submesh : public Mesh
	{
	public:
		void Render(HazelMesh* parentMesh, ::Ref<Shader> shader, glm::mat4 transform, uint32_t samplerSlot,
			const std::map<std::string, ::Ref<EnvMapMaterial>>& envMapMaterials);

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
		HazelMesh(const std::string& filename, ::Ref<Shader> shader, Material* material, bool isAnimated);
		virtual ~HazelMesh() override;

		virtual void Create() override;
		virtual void OnUpdate(float ts, bool debug) override;
		void OnImGuiRender(uint32_t id = 0);
		void DumpVertexBuffer();

		void Render(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, ::Ref<EnvMapMaterial>>& envMapMaterials);
		void RenderSubmeshes(uint32_t samplerSlot, const glm::mat4& transform, const std::map<std::string, ::Ref<EnvMapMaterial>>& envMapMaterials);

		// Getters
		std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }
		const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }

		inline const std::vector<Material*>& GetMaterials() const { return m_Materials; }
		inline const std::vector<Texture*>& GetTextures() const { return m_Textures; }
		inline bool& IsAnimated() { return m_IsAnimated; }
		inline Ref<VertexArray> GetVertexArray() { return m_VertexArray; }
		inline const std::vector<glm::mat4>& GetBoneTransforms() { return m_BoneTransforms; }
		const std::vector<Triangle> GetTriangleCache(uint32_t index) const;

		// Setters
		inline void SetBaseMaterial(Material* baseMaterial) { m_BaseMaterial = baseMaterial; }
		inline void SetTimeMultiplier(float timeMultiplier) { m_TimeMultiplier = timeMultiplier; }

		static std::string GetSubmeshMaterialName(Ref<HazelMesh> mesh, Hazel::Submesh& submesh);

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
		Texture* LoadBaseTexture();

	public:
		Ref<VertexArray> m_VertexArray;
		OpenGLIndexBuffer* m_IndexBuffer;
		std::vector<glm::mat4> m_BoneTransforms;
		std::vector<Submesh> m_Submeshes;

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
		::Ref<Shader> m_MeshShader;
		Material* m_BaseMaterial; // TODO: Convert m_BaseMaterial type to Hazel/Renderer/HazelMaterial
		Texture* m_BaseTexture;
		std::vector<Texture*> m_Textures;
		std::vector<Texture*> m_NormalMaps;
		std::vector<Material*> m_Materials;

		std::unordered_map<uint32_t, std::vector<Triangle>> m_TriangleCache;

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
