#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"

#include "EnvMapVulkanAntialiasingMode.h"

#include <vulkan/vulkan.h>


namespace H2M
{
	class FramebufferH2M;
	class IndexBufferH2M;
	class PipelineH2M;
	class Texture2D_H2M;
	class VertexBufferH2M;
}

/**
 * Antialiasing of a finished picture (docs/engine/Game_Engine_Runtime_Guide.html, 8.1): after the composite (exposure,
 * bloom, tonemapping), before the UI is drawn over it. SMAA 1x or FXAA, on the gamma encoded picture.
 *
 * A picture being smoothed has a Target: the composite draws into its Input instead of its usual framebuffer, then
 * RecordPrepasses draws SMAA's edges and weights, and RecordResolve draws the smoothed picture in the usual framebuffer's
 * render pass, where the UI goes on. Off: no Target images, and nothing of this costs anything.
 *
 * One object serves the main view and the texture views: their framebuffers all have the formats of the main composite
 * (RGBA8 + depth), so the pipelines fit each of them.
 */
class EnvMapVulkanAntialiasing
{
public:
	struct Target
	{
		H2M::RefH2M<H2M::FramebufferH2M> Input; // what the composite draws into

	private:
		friend class EnvMapVulkanAntialiasing;
		H2M::RefH2M<H2M::FramebufferH2M> Edges;   // SMAA only
		H2M::RefH2M<H2M::FramebufferH2M> Weights; // SMAA only
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet EdgesSet, WeightsSet, BlendSet, FXAASet;
	};

	// What RecordResolve draws: the debug views show SMAA's passes
	enum class Output { Picture = 0, Edges = 1, Weights = 2 };

	// The full-screen quad the composite draws with (the renderer's: position, texture coordinate)
	void Create(const H2M::RefH2M<H2M::VertexBufferH2M>& quadVertexBuffer, const H2M::RefH2M<H2M::IndexBufferH2M>& quadIndexBuffer);
	void Destroy();

	// Before anything of the frame is recorded: the target's images for this mode and size. Any change waits for the GPU.
	void Prepare(Target& target, AntialiasingMode mode, uint32_t width, uint32_t height);
	// Frees a target's images and sets (Off does the same through Prepare)
	void Release(Target& target);

	// Outside a render pass, after the composite was drawn into target.Input
	void RecordPrepasses(VkCommandBuffer commandBuffer, Target& target, AntialiasingMode mode);
	// Inside the render pass of the picture's usual framebuffer, with its viewport
	void RecordResolve(VkCommandBuffer commandBuffer, Target& target, AntialiasingMode mode, Output output = Output::Picture);

private:
	void RecordPass(VkCommandBuffer commandBuffer, const H2M::RefH2M<H2M::FramebufferH2M>& framebuffer,
		const H2M::RefH2M<H2M::PipelineH2M>& pipeline, const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set);
	void DrawQuad(VkCommandBuffer commandBuffer, const H2M::RefH2M<H2M::PipelineH2M>& pipeline,
		const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set);
	void WriteSets(Target& target);

	H2M::RefH2M<H2M::VertexBufferH2M> m_QuadVertexBuffer;
	H2M::RefH2M<H2M::IndexBufferH2M> m_QuadIndexBuffer;
	H2M::RefH2M<H2M::PipelineH2M> m_EdgesPipeline;
	H2M::RefH2M<H2M::PipelineH2M> m_WeightsPipeline;
	H2M::RefH2M<H2M::PipelineH2M> m_BlendPipeline;
	H2M::RefH2M<H2M::PipelineH2M> m_FXAAPipeline;
	H2M::RefH2M<H2M::Texture2D_H2M> m_AreaTexture;
	H2M::RefH2M<H2M::Texture2D_H2M> m_SearchTexture;
	VkDescriptorImageInfo m_AreaInfo = {};   // the lookup textures with a clamping sampler
	VkDescriptorImageInfo m_SearchInfo = {};
};
