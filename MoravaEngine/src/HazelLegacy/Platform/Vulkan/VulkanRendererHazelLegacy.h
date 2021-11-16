#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Platform/Vulkan/VulkanMaterial.h"
#include "Hazel/Renderer/HazelCamera.h"
#include "Hazel/Renderer/RendererCapabilities.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "HazelLegacy/Renderer/MeshHazelLegacy.h"

#include "Scene/Scene.h"
#include "Core/Window.h"


namespace Hazel {

	class VulkanRendererHazelLegacy : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false) override;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) override;
		virtual void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<HazelMaterial> material) override;
		virtual void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMaterial> material) override;
		virtual void SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<HazelMaterial> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides) override;

		virtual void SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<HazelImage2D> shadow, Ref<HazelImage2D> linearDepth) override;
		virtual std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath) override;
		virtual Ref<HazelTextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) override;

		virtual void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform) override;
		virtual void RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMesh> mesh, Ref<HazelMaterial> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) override;

		virtual void RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMaterial> material, const glm::mat4& transform) override;
		virtual void LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> pipelineCompute, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMaterial> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups) override;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMaterial> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount = 0) override;
		virtual void DispatchComputeShader(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<HazelMaterial> material, const glm::ivec3& workGroups) override;
		virtual void ClearImage(Ref<RenderCommandBuffer> commandBuffer, Ref<HazelImage2D> image) override;

		virtual RendererCapabilities& GetCapabilities() override;

		static void RenderMeshVulkan(Ref<MeshHazelLegacy> mesh, VkCommandBuffer commandBuffer);
		static void RenderSkybox(VkCommandBuffer commandBuffer);

		static void Draw(Scene* scene); // TODO: there should be no parameters
		static void GeometryPass();
		static void CompositePass();
		static void OnImGuiRender(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers);
		static glm::vec3 GetLightDirectionTemp();
		static void RT_UpdateMaterialForRendering(Ref<VulkanMaterial> material, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet);
		static VkSampler GetClampSampler();

		// static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
		static void UpdateImGuizmo(Window* mainWindow);
		static uint32_t GetDescriptorAllocationCount(uint32_t frameIndex = 0);

		static int32_t& GetSelectedDrawCall();

		static void SetCamera(HazelCamera& camera);

		/**** BEGIN methods moved from VulkanTestLayer to VulkanRenderer ****/
		static SceneRendererOptions& GetOptions(); // moved from VulkanTestLayer to VulkanRenderer
		static void MapUniformBuffersVTL(Ref<MeshHazelLegacy> mesh, const EditorCamera& camera);
		/**** END methods moved from VulkanTestLayer to VulkanRenderer ****/

		static void SubmitMeshTemp(const Ref<MeshHazelLegacy>& mesh, const glm::mat4& transform = glm::mat4(1.0f)); // to be removed from VulkanRenderer
		static void OnResize(uint32_t width, uint32_t height);                                                // to be removed from VulkanRenderer
		static uint32_t GetViewportWidth();                                                                   // to be removed from VulkanRenderer
		static uint32_t GetViewportHeight();                                                                  // to be removed from VulkanRenderer

		// Obsolete methods
		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<MeshHazelLegacy> mesh, const glm::mat4& transform) override;
		virtual void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<MeshHazelLegacy> mesh, const glm::mat4& transform) override;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform) override;

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
