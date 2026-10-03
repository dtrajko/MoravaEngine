/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/CameraH2M.h"
#include "H2M/Renderer/ModelH2M.h"
#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RendererCapabilitiesH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"

#include "Core/Window.h"


class EnvMapVulkanRenderer : public H2M::RendererAPI_H2M
{
public:
	virtual void Init() override;
	virtual void Shutdown() override;

	virtual void BeginFrame() override;
	virtual void EndFrame() override;

	virtual void BeginRenderPass(const H2M::RefH2M<H2M::RenderPassH2M>& renderPass) override;
	virtual void EndRenderPass() override;
	virtual void SubmitFullscreenQuad(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MaterialH2M> material) override;

	virtual void SetSceneEnvironment(H2M::RefH2M<H2M::EnvironmentH2M> environment, H2M::RefH2M<H2M::Image2D_H2M> shadow) override;

	virtual void RenderMesh(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform) override;
	virtual void RenderMeshWithoutMaterial(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform) override;
	virtual void RenderQuad(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MaterialH2M> material, const glm::mat4& transform) override;

	virtual void DrawIndexed(uint32_t indexCount, H2M::PrimitiveTypeH2M type, bool depthTest = true) override;
	virtual void DrawLines(H2M::RefH2M<H2M::VertexArrayH2M> vertexArray, uint32_t vertexCount) override;

	virtual void SetLineWidth(float width) override;

	virtual std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath) override;

	virtual H2M::RendererCapabilitiesH2M GetCapabilities() override;

	// materials: the Material Library material of each mesh (same order as the model's meshes)
	static void SubmitModelTemp(const H2M::RefH2M<H2M::ModelH2M>& model, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials); // to be removed from VulkanRendererH2M
	static void OnResize(uint32_t width, uint32_t height);                                                 // to be removed from VulkanRendererH2M
	static uint32_t GetViewportWidth();                                                                    // to be removed from VulkanRendererH2M
	static uint32_t GetViewportHeight();                                                                   // to be removed from VulkanRendererH2M

	static void RenderModelVulkan(H2M::RefH2M<H2M::ModelH2M> model, const glm::mat4& transform, const std::vector<H2M::RefH2M<EnvMapVulkanMaterial>>& materials, VkCommandBuffer commandBuffer);

	static void RenderSkybox(VkCommandBuffer commandBuffer);

	static void Draw(H2M::CameraH2M* camera); // TODO: there should be no parameters
	// The environment map (HDR) loaded at startup: set by the scene before Init (SceneEnvMapVulkan, from its user preferences)
	static void SetEnvironmentMapFile(const std::string& filepath);
	static void ViewportCompositePass(VkCommandBuffer commandBuffer);
	static void RenderGrid(VkCommandBuffer commandBuffer);
	static void GeometryPass();
	static void CompositePass();
	static void OnImGuiRender(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers);

	// static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
	static void UpdateImGuizmo(Window* mainWindow);

	static int32_t& GetSelectedDrawCall();

	static void SetCamera(H2M::CameraH2M& camera);

	/**** BEGIN methods moved from VulkanTestLayer to VulkanRendererH2M ****/
	static H2M::SceneRendererOptionsH2M& GetOptions(); // moved from VulkanTestLayer to VulkanRendererH2M
	static void MapUniformBuffersVTL(H2M::RefH2M<H2M::ModelH2M> model, const H2M::EditorCameraH2M& camera);
	/**** END methods moved from VulkanTestLayer to VulkanRendererH2M ****/

public:
	static bool s_MipMapsEnabled;
	static bool s_ViewportFBNeedsResize;

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
