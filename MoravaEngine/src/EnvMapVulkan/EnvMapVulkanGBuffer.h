#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"
#include "H2M/Renderer/PipelineH2M.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>


/**
 * G-buffer (SceneEnvMapVulkan): per-pixel surface data written by a prepass before the scene pass (Phase A of
 * docs/rendering/SSAO_GI_RayTracing_Guide.html). SSAO, the probes' debug views and later the ray-traced effects read it,
 * and the scene pass tests against its depth (EQUAL), so the PBR shader runs once per pixel.
 *
 * Attachments of the prepass render pass:
 *   0  normal + roughness  RGBA16F  xyz = world-space shading normal (after the normal map), w = roughness; cleared to 0
 *                                   (a zero normal: no mesh, the sky)
 *   1  motion              RG16F    screen-space motion in UV units, this frame's position minus the previous one's
 *   2  depth               the scene framebuffer's depth image (this module only makes a view of it), cleared to 1
 *
 * The prepass pipelines (GBufferPrepass_Static.glsl, GBufferPrepass_Anim.glsl) share the vertex stages of the PBR shaders
 * and are created with the PBR pipelines' layouts and vertex layouts: the renderer draws the meshes into the G-buffer with
 * the same frame, material and bone sets and push constants as in the scene pass (RenderModelVulkan, MeshPass::GBuffer).
 *
 * Owns: the normal and motion images, the render pass (created once: the formats don't change), the view of the scene's
 * depth, the framebuffer and the two prepass pipelines (not their layouts: those belong to the PBR pipelines). After the render pass the normal and motion images are in SHADER_READ_ONLY_OPTIMAL and the
 * depth in DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
 */
class EnvMapVulkanGBuffer
{
public:
	static constexpr VkFormat NormalRoughnessFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	static constexpr VkFormat MotionFormat = VK_FORMAT_R16G16_SFLOAT;
	static constexpr uint32_t ColorAttachmentCount = 2;

	void Create(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer);
	// The prepass pipelines for static and skinned meshes, with the layouts of these PBR pipelines (which must outlive them)
	void CreatePipelines(H2M::RefH2M<H2M::PipelineH2M> staticMeshPipeline, H2M::RefH2M<H2M::PipelineH2M> skinnedMeshPipeline);
	void Destroy();
	// Follows the scene framebuffer: call after it was resized. Rebuilds when the size or the scene's depth image changed
	// (waits for the GPU: the old images may be in use).
	void Resize(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer);

	// The prepass render pass, with its viewport and scissor set: clears the normal, motion and depth attachments
	void BeginPass(VkCommandBuffer commandBuffer);
	void EndPass(VkCommandBuffer commandBuffer);

	VkRenderPass GetRenderPass() const { return m_RenderPass; }
	VkPipeline GetPipeline(bool skinned) const { return skinned ? m_SkinnedPipeline : m_StaticPipeline; }
	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
	// For descriptor sets that sample the G-buffer (nearest filtering, SHADER_READ_ONLY_OPTIMAL)
	VkDescriptorImageInfo GetNormalRoughnessInfo() const { return { m_Sampler, m_NormalRoughness.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }; }
	VkDescriptorImageInfo GetMotionInfo() const { return { m_Sampler, m_Motion.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }; }

private:
	struct Attachment
	{
		VkImage Image = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		VkImageView View = VK_NULL_HANDLE;
	};
	void CreateRenderPass();
	VkPipeline CreatePipeline(const std::string& shaderName, H2M::RefH2M<H2M::PipelineH2M> meshPipeline);
	void CreateAttachment(Attachment& attachment, VkFormat format);
	void DestroyTargets(); // the images, the depth view and the framebuffer (not the render pass or the sampler)

	Attachment m_NormalRoughness, m_Motion;
	VkImage m_SceneDepthImage = VK_NULL_HANDLE; // the scene framebuffer's, not owned (a new one means a rebuild)
	VkImageView m_DepthView = VK_NULL_HANDLE;
	VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
	VkSampler m_Sampler = VK_NULL_HANDLE;
	VkPipeline m_StaticPipeline = VK_NULL_HANDLE;
	VkPipeline m_SkinnedPipeline = VK_NULL_HANDLE;
	uint32_t m_Width = 0, m_Height = 0;
};
