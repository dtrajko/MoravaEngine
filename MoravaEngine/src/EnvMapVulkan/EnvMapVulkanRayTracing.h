#pragma once

#include "EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkanTemporalFilter.h"
#include "EnvMapVulkanTextureTable.h"

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/ModelH2M.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>


/**
 * The scene as rays see it (SceneEnvMapVulkan, Phase G of docs/rendering/SSAO_GI_RayTracing_Guide.html): acceleration
 * structures over the meshes, and everything a compute shader needs to trace a ray (a ray query) and read the surface
 * and the material it hit. The ray-traced effects (Phase H) are built on this; so far it shows the ray-traced debug
 * views.
 *
 * Needs a GPU with ray tracing (IsSupported). Without one, Create only makes the debug view's image (black), and
 * everything else does nothing.
 *
 * Acceleration structures:
 *   - One bottom-level structure (BLAS) per mesh: the triangles of one part of a model, in the model's own space. Built
 *     once, when the mesh is first submitted (it waits for the GPU), and made smaller (compacted). Models that share a
 *     mesh share its structure.
 *   - A skinned model's vertices are moved by its bones in a compute pass every frame (RayTracingSkinning.glsl), into a
 *     buffer of its own; its meshes' structures are refitted to the new positions (an update: much cheaper than a build).
 *   - One top-level structure (TLAS), built every frame the rays are used: an instance per submitted mesh, with the
 *     transform the mesh is drawn with, a group (the ray mask: opaque or glass) and its number in the instance table.
 *
 * The ray tracing set (set 3 of a shader that includes Include/RayTracing.glslh, GetDescriptorSet): the TLAS, the
 * instance table (each mesh's vertices, indices and material), the material buffer and the array of all textures
 * (GetTextureTable, filled by the material buffer).
 *
 * A frame: BeginFrame, Submit for every model that is drawn, then Record in the frame's command buffer, before any pass
 * that traces rays. RecordView draws a debug view into the view image.
 *
 * Owns: the structures and their buffers, the skinned vertex buffers, the instance buffers, the texture table, the two
 * compute pipelines, their descriptor sets and the view image.
 */
class EnvMapVulkanRayTracing
{
public:
	static constexpr VkFormat ViewFormat = VK_FORMAT_R8G8B8A8_UNORM;
	// The groups an instance can be in (a ray chooses which it can hit): RayMask... in Include/RayTracing.glslh
	static constexpr uint32_t MaskOpaqueStatic = 0x01;  // opaque meshes of the models that stand still
	static constexpr uint32_t MaskGlass = 0x02;         // glass meshes
	static constexpr uint32_t MaskOpaqueSkinned = 0x04; // the meshes of skinned models (all drawn as opaque)
	static constexpr uint32_t MaskGlassShadow = 0x08;   // glass whose material casts a shadow (with MaskGlass)
	static constexpr uint32_t MaskOpaque = MaskOpaqueStatic | MaskOpaqueSkinned;

	// What the view image shows (RayTracingView.glsl)
	enum class View { Normals = 0, Albedo, Instances };

	// An entry of the instance table. std430, 24 bytes: must match RayTracingInstance in Include/RayTracing.glslh
	struct GPUInstance
	{
		uint64_t VertexAddress = 0; // the mesh's first vertex (56 bytes each: H2M::VertexH2M)
		uint64_t IndexAddress = 0;  // the mesh's first index
		uint32_t MaterialIndex = 0; // the mesh's entry in the material buffer
		uint32_t Flags = 0;
	};

	// The push constants of RayTracingView.glsl
	struct ViewPush
	{
		glm::mat4 InverseViewProjection;
		glm::ivec4 Params;
	};

	struct Stats
	{
		uint32_t Instances = 0;       // meshes in the TLAS this frame
		uint32_t Meshes = 0;          // bottom-level structures alive
		uint32_t SkinnedModels = 0;   // skinned and refitted this frame
		uint64_t Triangles = 0;       // in the instances
		VkDeviceSize StructureBytes = 0; // the bottom-level structures and the TLAS
		uint32_t Textures = 0;        // in the array of all textures
	};

	// The device has the ray tracing extensions and features enabled
	static bool IsSupported();

	void Create();
	void Destroy();

	// The array of all textures, for the material buffer's map indices. Null without ray tracing.
	EnvMapVulkanTextureTable* GetTextureTable() { return m_Created ? &m_Textures : nullptr; }
	// Writes the textures that were given an element since the last call into the set (after the material buffer's Update)
	void UpdateTextures();
	// The material buffer for the set: at startup, and when a bigger buffer replaced it (the caller waited for the GPU)
	void SetMaterialBuffer(const VkDescriptorBufferInfo& materialBuffer);

	void BeginFrame();
	// A model that is drawn this frame. meshTransforms: the world transform each of its meshes is drawn with; materials:
	// each mesh's material (a mesh without one is left out, as in the mesh passes).
	void Submit(const H2M::RefH2M<H2M::ModelH2M>& model, const std::vector<glm::mat4>& meshTransforms,
		const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials);
	// New meshes get their structures (waits for the GPU), the skinned models are skinned and refitted, the TLAS is built
	// from the submitted meshes. Afterwards compute shaders can trace rays against this frame's scene.
	void Record(VkCommandBuffer commandBuffer);

	// Set 3 of the shaders that trace rays
	VkDescriptorSet GetDescriptorSet() const { return m_SceneSet.DescriptorSets.empty() ? VK_NULL_HANDLE : m_SceneSet.DescriptorSets[0]; }

	// The view image follows the viewport's size (the old image must no longer be in use). It is a new image then: the
	// set that shows it must be written again (GetViewInfo).
	void ResizeView(uint32_t width, uint32_t height);
	// After Record: a ray per pixel from the camera into the view image
	void RecordView(VkCommandBuffer commandBuffer, View view, const glm::mat4& viewProjection);
	VkDescriptorImageInfo GetViewInfo() const { return m_ViewImage.GetSampledInfo(m_Sampler); }

	const Stats& GetStats() const { return m_Stats; }

private:
	struct Buffer
	{
		VkBuffer Handle = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		VkDeviceSize Size = 0;
		VkDeviceAddress Address = 0;
		void* Mapped = nullptr; // host-visible buffers stay mapped

		void Create(VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible);
		void Destroy();
	};

	struct Structure
	{
		VkAccelerationStructureKHR Handle = VK_NULL_HANDLE;
		Buffer Storage;
		VkDeviceAddress Address = 0; // what a TLAS instance refers to

		void Create(VkDeviceSize size, VkAccelerationStructureTypeKHR type);
		void Destroy();
	};

	// A skinned model: its vertices in this frame's pose
	struct SkinnedModel
	{
		H2M::RefH2M<H2M::ModelH2M> Model;
		Buffer Vertices; // H2M::VertexH2M, one per vertex of the model
		uint32_t VertexCount = 0;
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet Set; // the skinning pass's: bones, source, destination
		uint64_t LastUsedFrame = 0;
	};

	// A mesh's bottom-level structure
	struct MeshEntry
	{
		H2M::RefH2M<H2M::MeshH2M> Mesh; // keeps the key (its address) from being reused while the entry lives
		Structure BLAS;
		VkDeviceAddress VertexAddress = 0; // the mesh's first vertex: in the model's vertex buffer, or in the skinned buffer
		VkDeviceAddress IndexAddress = 0;  // the mesh's first index in the model's index buffer
		uint32_t MaxVertex = 0;
		uint32_t TriangleCount = 0;
		SkinnedModel* Skinned = nullptr;   // the model whose bones move it (null: a static mesh)
		VkDeviceSize ScratchSize = 0;      // skinned: for a build or an update
		bool Built = false;                // skinned: built once, from then on updated
		uint64_t LastUsedFrame = 0;
	};

	struct FrameInstance
	{
		H2M::RefH2M<H2M::ModelH2M> Model;
		H2M::RefH2M<H2M::MeshH2M> Mesh;
		glm::mat4 Transform;
		uint32_t MaterialIndex = 0;
		uint32_t Mask = MaskOpaqueStatic;
	};

	MeshEntry* GetMeshEntry(const FrameInstance& instance, std::vector<MeshEntry*>& newStaticMeshes);
	SkinnedModel& GetSkinnedModel(const H2M::RefH2M<H2M::ModelH2M>& model);
	void BuildStaticMeshes(const std::vector<MeshEntry*>& meshes);
	void RetireUnused();
	void CreateTopLevel(uint32_t instanceCapacity);
	void DestroyTopLevel();
	void WriteSceneDescriptors();
	void ReserveScratch(VkDeviceSize size);

	bool m_Created = false; // with ray tracing
	uint64_t m_Frame = 0;   // counts the calls of Record

	std::unordered_map<const H2M::MeshH2M*, MeshEntry> m_Meshes;
	std::unordered_map<const H2M::ModelH2M*, SkinnedModel> m_SkinnedModels;
	std::vector<FrameInstance> m_FrameInstances;

	Structure m_TLAS;
	uint32_t m_InstanceCapacity = 0;
	VkDeviceSize m_TLASScratchSize = 0;
	Buffer m_InstanceInput; // VkAccelerationStructureInstanceKHR per instance: what the TLAS is built from
	Buffer m_InstanceTable; // GPUInstance per instance: what a hit reads
	Buffer m_Scratch;       // working memory of the frame's builds and updates
	VkDeviceSize m_ScratchAlignment = 1;

	EnvMapVulkanTextureTable m_Textures;
	VkDescriptorBufferInfo m_MaterialBuffer = {};

	H2M::RefH2M<H2M::VulkanComputePipelineH2M> m_SkinningPipeline;
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> m_ViewPipeline;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_SceneSet; // set 3: TLAS, instance table, material buffer, textures
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_ViewSet;  // the view pass's set 0: the view image

	EnvMapVulkanComputeImage m_ViewImage;
	VkSampler m_Sampler = VK_NULL_HANDLE; // nearest
	Stats m_Stats;
};
