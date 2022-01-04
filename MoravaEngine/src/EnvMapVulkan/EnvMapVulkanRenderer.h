/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/CameraH2M.h"
#include "H2M/Renderer/MeshH2M.h"
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

	virtual void RenderMesh(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform) override;
	virtual void RenderMeshWithoutMaterial(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform) override;
	virtual void RenderQuad(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MaterialH2M> material, const glm::mat4& transform) override;

	virtual void DrawIndexed(uint32_t indexCount, H2M::PrimitiveTypeH2M type, bool depthTest = true) override;
	virtual void DrawLines(H2M::RefH2M<H2M::VertexArrayH2M> vertexArray, uint32_t vertexCount) override;

	virtual void SetLineWidth(float width) override;

	virtual std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath) override;

	virtual H2M::RendererCapabilitiesH2M& GetCapabilities() override;

	static void SubmitMeshTemp(const H2M::RefH2M<H2M::MeshH2M>& mesh, const glm::mat4& transform = glm::mat4(1.0f)); // to be removed from VulkanRendererH2M
	static void OnResize(uint32_t width, uint32_t height);                                                 // to be removed from VulkanRendererH2M
	static uint32_t GetViewportWidth();                                                                    // to be removed from VulkanRendererH2M
	static uint32_t GetViewportHeight();                                                                   // to be removed from VulkanRendererH2M

	static void RenderMeshVulkan(H2M::RefH2M<H2M::MeshH2M> mesh, VkCommandBuffer commandBuffer);

	static void RenderSkybox(VkCommandBuffer commandBuffer);

	static void Draw(H2M::CameraH2M* camera); // TODO: there should be no parameters
	static void GeometryPass();
	static void CompositePass();
	static void OnImGuiRender(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers);
	static glm::vec3 GetLightDirectionTemp();

	// static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
	static void UpdateImGuizmo(Window* mainWindow);

	static int32_t& GetSelectedDrawCall();

	static void SetCamera(H2M::CameraH2M& camera);

	/**** BEGIN methods moved from VulkanTestLayer to VulkanRendererH2M ****/
	static H2M::SceneRendererOptionsH2M& GetOptions(); // moved from VulkanTestLayer to VulkanRendererH2M
	static void MapUniformBuffersVTL(H2M::RefH2M<H2M::MeshH2M> mesh, const H2M::EditorCameraH2M& camera);
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
