#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Platform/Vulkan/VulkanMaterial.h"
#include "H2M/Renderer/HazelCamera.h"
#include "H2M/Renderer/RendererAPI.h"
#include "H2M/Renderer/RendererCapabilities.h"
#include "H2M/Renderer/SceneRenderer.h"

#include "H2M/Renderer/MeshH2M.h"

#include "Scene/Scene.h"
#include "Core/Window.h"


namespace Hazel {

	class VulkanRendererH2M : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void BeginRenderPass(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<RenderPass> renderPass, bool explicitClear = false) override;
		virtual void EndRenderPass(RefH2M<RenderCommandBuffer> renderCommandBuffer) override;
		virtual void SubmitFullscreenQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<HazelMaterial> material) override;
		virtual void SubmitFullscreenQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMaterial> material) override;
		virtual void SubmitFullscreenQuadWithOverrides(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<HazelMaterial> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides) override;

		virtual void SetSceneEnvironment(RefH2M<SceneRenderer> sceneRenderer, RefH2M<Environment> environment, RefH2M<HazelImage2D> shadow, RefH2M<HazelImage2D> linearDepth) override;
		virtual std::pair<RefH2M<TextureCubeH2M>, RefH2M<TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath) override;
		virtual RefH2M<TextureCubeH2M> CreatePreethamSky(float turbidity, float azimuth, float inclination) override;

		virtual void RenderMesh(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMesh> mesh, RefH2M<MaterialTable> materialTable, const glm::mat4& transform) override {};
		virtual void RenderMeshWithMaterial(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMesh> mesh, RefH2M<HazelMaterial> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) override {};
		virtual void RenderQuad(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMaterial> material, const glm::mat4& transform) override;
		virtual void LightCulling(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<PipelineCompute> pipelineCompute, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMaterial> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups) override;
		virtual void RenderGeometry(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMaterial> material, RefH2M<VertexBuffer> vertexBuffer, RefH2M<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount = 0) override;
		virtual void DispatchComputeShader(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<PipelineCompute> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<HazelMaterial> material, const glm::ivec3& workGroups) override;
		virtual void ClearImage(RefH2M<RenderCommandBuffer> commandBuffer, RefH2M<HazelImage2D> image) override;

		virtual RendererCapabilities& GetCapabilities() override;

		static void RenderMeshVulkan(RefH2M<MeshH2M> mesh, VkCommandBuffer commandBuffer);
		static void RenderSkybox(VkCommandBuffer commandBuffer);

		static void Draw(Scene* scene); // TODO: there should be no parameters
		static void GeometryPass();
		static void CompositePass();
		static void OnImGuiRender(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers);
		static glm::vec3 GetLightDirectionTemp();
		static void RT_UpdateMaterialForRendering(RefH2M<VulkanMaterial> material, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet);
		static VkSampler GetClampSampler();

		// static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
		static void UpdateImGuizmo(Window* mainWindow);
		static uint32_t GetDescriptorAllocationCount(uint32_t frameIndex = 0);

		static int32_t& GetSelectedDrawCall();

		static void SetCamera(HazelCamera& camera);

		/**** BEGIN methods moved from VulkanTestLayer to VulkanRenderer ****/
		static SceneRendererOptions& GetOptions(); // moved from VulkanTestLayer to VulkanRenderer
		static void MapUniformBuffersVTL(RefH2M<MeshH2M> mesh, const EditorCamera& camera);
		/**** END methods moved from VulkanTestLayer to VulkanRenderer ****/

		static void SubmitMeshTemp(const RefH2M<MeshH2M>& mesh, const glm::mat4& transform = glm::mat4(1.0f)); // to be removed from VulkanRenderer
		static void OnResize(uint32_t width, uint32_t height);                                                // to be removed from VulkanRenderer
		static uint32_t GetViewportWidth();                                                                   // to be removed from VulkanRenderer
		static uint32_t GetViewportHeight();                                                                  // to be removed from VulkanRenderer

		// Obsolete methods
		virtual void RenderMesh(RefH2M<Pipeline> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform) override;
		virtual void RenderMeshWithoutMaterial(RefH2M<Pipeline> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform) override;
		virtual void RenderQuad(RefH2M<Pipeline> pipeline, RefH2M<HazelMaterial> material, const glm::mat4& transform) override;

		void RenderMesh(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<MeshH2M> mesh, RefH2M<MaterialTable> materialTable, const glm::mat4& transform);
		void RenderMeshWithMaterial(RefH2M<RenderCommandBuffer> renderCommandBuffer, RefH2M<Pipeline> pipeline, RefH2M<UniformBufferSet> uniformBufferSet, RefH2M<StorageBufferSet> storageBufferSet, RefH2M<MeshH2M> mesh, RefH2M<HazelMaterial> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer());

	public:
		static bool s_MipMapsEnabled;
		static bool s_ViewportFBNeedsResize;

		static Scene* s_Scene;
		static VkDescriptorSet RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo);
	};

	namespace Utils {

		void InsertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange);

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}

}
