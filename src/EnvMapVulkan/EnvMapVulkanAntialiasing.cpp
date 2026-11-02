#include "EnvMapVulkanAntialiasing.h"

#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanIndexBufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Platform/Vulkan/VulkanVertexBufferH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"
#include "H2M/Renderer/PipelineH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/RenderPassH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include "EnvMapVulkanProfiler.h"

#include "smaa/AreaTex.h"
#include "smaa/SearchTex.h"

#include <algorithm>
#include <vector>


static VkDevice GetDevice()
{
	return H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
}

static H2M::RefH2M<H2M::FramebufferH2M> CreateFramebuffer(uint32_t width, uint32_t height, const char* name, H2M::ImageFormatH2M format = H2M::ImageFormatH2M::RGBA)
{
	H2M::FramebufferSpecificationH2M spec;
	spec.Attachments = { format, H2M::ImageFormatH2M::Depth }; // RGBA8: as the viewport composite's
	spec.Samples = 1;
	spec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	spec.Width = width;
	spec.Height = height;
	spec.DebugName = name;
	return H2M::FramebufferH2M::Create(spec);
}

// The lookup textures have 1 and 2 channels; the renderer's textures here 4
static H2M::RefH2M<H2M::Texture2D_H2M> CreateLookupTexture(const unsigned char* bytes, uint32_t width, uint32_t height, uint32_t channels)
{
	std::vector<unsigned char> pixels((size_t)width * height * 4);
	for (size_t i = 0; i < (size_t)width * height; i++)
	{
		pixels[i * 4 + 0] = bytes[i * channels];
		pixels[i * 4 + 1] = channels > 1 ? bytes[i * channels + 1] : 0;
		pixels[i * 4 + 2] = 0;
		pixels[i * 4 + 3] = 255;
	}
	return H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::RGBA, width, height, pixels.data());
}

static void FreeSet(H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set)
{
	if (set.Pool)
	{
		vkDestroyDescriptorPool(GetDevice(), set.Pool, nullptr);
	}
	set = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
}

// The radical inverse in a base: 0.5, 0.25, 0.75... for base 2
static float Halton(uint32_t index, uint32_t base)
{
	float result = 0.0f;
	float fraction = 1.0f;
	while (index > 0)
	{
		fraction /= (float)base;
		result += fraction * (float)(index % base);
		index /= base;
	}
	return result;
}

glm::vec2 EnvMapVulkanAntialiasing::GetJitter(uint64_t frame, uint32_t width, uint32_t height)
{
	const uint32_t index = (uint32_t)(frame % 8) + 1; // from 1: the sequence's 0 is the same corner for both bases
	const glm::vec2 pixels(Halton(index, 2) - 0.5f, Halton(index, 3) - 0.5f);
	return pixels * 2.0f / glm::vec2((float)std::max(width, 1u), (float)std::max(height, 1u)); // clip space is 2 wide
}

void EnvMapVulkanAntialiasing::Create(const H2M::RefH2M<H2M::VertexBufferH2M>& quadVertexBuffer, const H2M::RefH2M<H2M::IndexBufferH2M>& quadIndexBuffer)
{
	m_QuadVertexBuffer = quadVertexBuffer;
	m_QuadIndexBuffer = quadIndexBuffer;

	H2M::RenderPassSpecificationH2M renderPassSpec;
	renderPassSpec.TargetFramebuffer = CreateFramebuffer(16, 16, "Antialiasing");
	H2M::RefH2M<H2M::RenderPassH2M> renderPass = H2M::RenderPassH2M::Create(renderPassSpec);

	H2M::RenderPassSpecificationH2M historyPassSpec;
	historyPassSpec.TargetFramebuffer = CreateFramebuffer(16, 16, "AntialiasingHistory", H2M::ImageFormatH2M::RGBA16F);
	H2M::RefH2M<H2M::RenderPassH2M> historyPass = H2M::RenderPassH2M::Create(historyPassSpec);

	auto createPipeline = [&](const char* shader, const H2M::RefH2M<H2M::RenderPassH2M>& pass)
	{
		H2M::PipelineSpecificationH2M specification;
		specification.Layout = {
			{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
			{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
		};
		specification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get(shader);
		specification.RenderPass = pass;
		specification.DepthTest = false;
		specification.DepthWrite = false;
		specification.Blend = false; // the weights' alpha is data
		specification.DebugName = shader;
		return H2M::PipelineH2M::Create(specification);
	};
	m_EdgesPipeline = createPipeline("AntialiasingSMAAEdges", renderPass);
	m_WeightsPipeline = createPipeline("AntialiasingSMAAWeights", renderPass);
	m_BlendPipeline = createPipeline("AntialiasingSMAABlend", renderPass);
	m_FXAAPipeline = createPipeline("AntialiasingFXAA", renderPass);
	m_TAAResolvePipeline = createPipeline("AntialiasingTAAResolve", historyPass);
	m_TAASharpenPipeline = createPipeline("AntialiasingTAASharpen", renderPass);

	m_AreaTexture = CreateLookupTexture(areaTexBytes, AREATEX_WIDTH, AREATEX_HEIGHT, 2);
	m_SearchTexture = CreateLookupTexture(searchTexBytes, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1);

	VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.maxAnisotropy = 1.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	const VkSampler clampSampler = H2M::VulkanShaderH2M::GetSampler(sampler); // shared: not destroyed here
	m_AreaInfo = m_AreaTexture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
	m_AreaInfo.sampler = clampSampler;
	m_SearchInfo = m_SearchTexture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
	m_SearchInfo.sampler = clampSampler;
}

void EnvMapVulkanAntialiasing::Destroy()
{
	m_EdgesPipeline = {};
	m_WeightsPipeline = {};
	m_BlendPipeline = {};
	m_FXAAPipeline = {};
	m_TAAResolvePipeline = {};
	m_TAASharpenPipeline = {};
	m_AreaTexture = {};
	m_SearchTexture = {};
	m_QuadVertexBuffer = {};
	m_QuadIndexBuffer = {};
}

void EnvMapVulkanAntialiasing::Release(Target& target)
{
	if (!target.Input)
	{
		return;
	}
	vkDeviceWaitIdle(GetDevice());
	FreeSet(target.EdgesSet);
	FreeSet(target.WeightsSet);
	FreeSet(target.BlendSet);
	FreeSet(target.FXAASet);
	ReleaseTemporal(target);
	target.Input = {};
	target.Edges = {};
	target.Weights = {};
}

void EnvMapVulkanAntialiasing::ReleaseTemporal(Target& target)
{
	for (uint32_t i = 0; i < 2; i++)
	{
		FreeSet(target.ResolveSet[i]);
		FreeSet(target.SharpenSet[i]);
		target.History[i] = {};
	}
	target.MotionView = VK_NULL_HANDLE;
	target.DepthView = VK_NULL_HANDLE;
	target.HasHistory = false;
}

void EnvMapVulkanAntialiasing::Prepare(Target& target, AntialiasingMode mode, uint32_t width, uint32_t height, const TemporalImages* temporal)
{
	if (mode == AntialiasingMode::Off)
	{
		Release(target);
		return;
	}

	const bool smaa = mode == AntialiasingMode::SMAA;
	const bool resize = target.Input && (target.Input->GetWidth() != width || target.Input->GetHeight() != height);
	const bool taa = mode == AntialiasingMode::TAA;
	if (target.Input && !resize && smaa == (bool)target.Edges && taa == (bool)target.History[0])
	{
		if (taa && (target.MotionView != temporal->Motion.imageView || target.DepthView != temporal->Depth.imageView))
		{
			vkDeviceWaitIdle(GetDevice()); // the G-buffer was rebuilt
			WriteTemporalSets(target, *temporal);
		}
		return;
	}

	vkDeviceWaitIdle(GetDevice());
	if (target.Input)
	{
		target.Input->Resize(width, height);
	}
	else
	{
		target.Input = CreateFramebuffer(width, height, "AntialiasingInput");
		target.FXAASet = m_FXAAPipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->CreateDescriptorSets();
	}
	if (smaa && !target.Edges)
	{
		target.Edges = CreateFramebuffer(width, height, "SMAAEdges");
		target.Weights = CreateFramebuffer(width, height, "SMAAWeights");
		target.EdgesSet = m_EdgesPipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->CreateDescriptorSets();
		target.WeightsSet = m_WeightsPipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->CreateDescriptorSets();
		target.BlendSet = m_BlendPipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->CreateDescriptorSets();
	}
	else if (!smaa && target.Edges)
	{
		FreeSet(target.EdgesSet);
		FreeSet(target.WeightsSet);
		FreeSet(target.BlendSet);
		target.Edges = {};
		target.Weights = {};
	}
	else if (target.Edges)
	{
		target.Edges->Resize(width, height);
		target.Weights->Resize(width, height);
	}
	if (taa && !target.History[0])
	{
		for (uint32_t i = 0; i < 2; i++)
		{
			target.History[i] = CreateFramebuffer(width, height, "TAAHistory", H2M::ImageFormatH2M::RGBA16F);
			target.ResolveSet[i] = m_TAAResolvePipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->CreateDescriptorSets();
			target.SharpenSet[i] = m_TAASharpenPipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->CreateDescriptorSets();
		}
	}
	else if (!taa && target.History[0])
	{
		ReleaseTemporal(target);
	}
	else if (taa)
	{
		target.History[0]->Resize(width, height);
		target.History[1]->Resize(width, height);
	}
	WriteSets(target);
	if (taa)
	{
		WriteTemporalSets(target, *temporal);
		target.HasHistory = false; // new images hold nothing yet
	}
}

void EnvMapVulkanAntialiasing::WriteSets(Target& target)
{
	auto info = [](const H2M::RefH2M<H2M::FramebufferH2M>& framebuffer)
	{
		return &framebuffer.As<H2M::VulkanFramebufferH2M>()->GetVulkanDescriptorInfo();
	};
	std::vector<VkWriteDescriptorSet> writes;
	auto add = [&](const H2M::RefH2M<H2M::PipelineH2M>& pipeline, const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set, const char* name,
		const VkDescriptorImageInfo* image)
	{
		VkWriteDescriptorSet write = *pipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->GetDescriptorSet(name);
		write.dstSet = set.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = image;
		writes.push_back(write);
	};

	add(m_FXAAPipeline, target.FXAASet, "u_ColorTexture", info(target.Input));
	if (target.Edges)
	{
		add(m_EdgesPipeline, target.EdgesSet, "u_ColorTexture", info(target.Input));
		add(m_WeightsPipeline, target.WeightsSet, "u_EdgesTexture", info(target.Edges));
		add(m_WeightsPipeline, target.WeightsSet, "u_AreaTexture", &m_AreaInfo);
		add(m_WeightsPipeline, target.WeightsSet, "u_SearchTexture", &m_SearchInfo);
		add(m_BlendPipeline, target.BlendSet, "u_ColorTexture", info(target.Input));
		add(m_BlendPipeline, target.BlendSet, "u_WeightsTexture", info(target.Weights));
		add(m_BlendPipeline, target.BlendSet, "u_EdgesTexture", info(target.Edges));
	}
	vkUpdateDescriptorSets(GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void EnvMapVulkanAntialiasing::WriteTemporalSets(Target& target, const TemporalImages& temporal)
{
	auto info = [](const H2M::RefH2M<H2M::FramebufferH2M>& framebuffer)
	{
		return &framebuffer.As<H2M::VulkanFramebufferH2M>()->GetVulkanDescriptorInfo();
	};
	std::vector<VkWriteDescriptorSet> writes;
	auto add = [&](const H2M::RefH2M<H2M::PipelineH2M>& pipeline, const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set, const char* name,
		const VkDescriptorImageInfo* image)
	{
		VkWriteDescriptorSet write = *pipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->GetDescriptorSet(name);
		write.dstSet = set.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = image;
		writes.push_back(write);
	};
	for (uint32_t i = 0; i < 2; i++)
	{
		// Writes History[i], so it reads the other one
		add(m_TAAResolvePipeline, target.ResolveSet[i], "u_ColorTexture", info(target.Input));
		add(m_TAAResolvePipeline, target.ResolveSet[i], "u_HistoryTexture", info(target.History[1 - i]));
		add(m_TAAResolvePipeline, target.ResolveSet[i], "u_MotionTexture", &temporal.Motion);
		add(m_TAAResolvePipeline, target.ResolveSet[i], "u_DepthTexture", &temporal.Depth);
		add(m_TAASharpenPipeline, target.SharpenSet[i], "u_ColorTexture", info(target.History[i]));
	}
	vkUpdateDescriptorSets(GetDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
	target.MotionView = temporal.Motion.imageView;
	target.DepthView = temporal.Depth.imageView;
}

void EnvMapVulkanAntialiasing::DrawQuad(VkCommandBuffer commandBuffer, const H2M::RefH2M<H2M::PipelineH2M>& pipeline,
	const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set)
{
	H2M::RefH2M<H2M::VulkanPipelineH2M> vulkanPipeline = pipeline.As<H2M::VulkanPipelineH2M>();
	VkBuffer vertexBuffer = m_QuadVertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, m_QuadIndexBuffer.As<H2M::VulkanIndexBufferH2M>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipelineLayout(), 0, 1, set.DescriptorSets.data(), 0, nullptr);
	vkCmdDrawIndexed(commandBuffer, m_QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
	EnvMapVulkanProfiler::CountDraw(m_QuadIndexBuffer->GetCount() / 3);
}

void EnvMapVulkanAntialiasing::RecordPass(VkCommandBuffer commandBuffer, const H2M::RefH2M<H2M::FramebufferH2M>& framebuffer,
	const H2M::RefH2M<H2M::PipelineH2M>& pipeline, const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set)
{
	H2M::RefH2M<H2M::VulkanFramebufferH2M> vulkanFramebuffer = framebuffer.As<H2M::VulkanFramebufferH2M>();
	const uint32_t width = vulkanFramebuffer->GetWidth();
	const uint32_t height = vulkanFramebuffer->GetHeight();

	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = vulkanFramebuffer->GetRenderPass();
	beginInfo.framebuffer = vulkanFramebuffer->GetVulkanFramebuffer();
	beginInfo.renderArea.extent = { width, height };
	beginInfo.clearValueCount = 2;
	beginInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	EnvMapVulkanProfiler::CountRenderPass(width, height);

	const VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	const VkRect2D scissor = { { 0, 0 }, { width, height } };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	DrawQuad(commandBuffer, pipeline, set);
	vkCmdEndRenderPass(commandBuffer);
}

void EnvMapVulkanAntialiasing::RecordClear(VkCommandBuffer commandBuffer, const H2M::RefH2M<H2M::FramebufferH2M>& framebuffer)
{
	H2M::RefH2M<H2M::VulkanFramebufferH2M> vulkanFramebuffer = framebuffer.As<H2M::VulkanFramebufferH2M>();
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = vulkanFramebuffer->GetRenderPass();
	beginInfo.framebuffer = vulkanFramebuffer->GetVulkanFramebuffer();
	beginInfo.renderArea.extent = { vulkanFramebuffer->GetWidth(), vulkanFramebuffer->GetHeight() };
	beginInfo.clearValueCount = 2;
	beginInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(commandBuffer);
}

void EnvMapVulkanAntialiasing::RecordPrepasses(VkCommandBuffer commandBuffer, Target& target, AntialiasingMode mode, const TemporalFrame* temporal)
{
	if (mode == AntialiasingMode::TAA)
	{
		EnvMapVulkanProfiler::Scope scope(commandBuffer, "TAA Resolve", "Antialiasing over the frames: this frame's picture mixed with "
			"the result of the frames before, read where each surface point was in the previous frame (the G-buffer's motion), and "
			"limited to the colors around the pixel in this frame, so that what changed leaves no ghost.");
		// Must match the Uniforms block of AntialiasingTAAResolve.glsl
		struct ResolvePush
		{
			glm::mat4 CurrentToPrevious;
			glm::vec2 JitterMotion;
			float HasHistory;
			float Padding;
		} push = { temporal->CurrentToPrevious, temporal->JitterMotion, target.HasHistory ? 1.0f : 0.0f, 0.0f };
		static_assert(sizeof(ResolvePush) == 80, "layout mismatch with the Uniforms block of AntialiasingTAAResolve.glsl");
		const uint32_t written = 1 - target.HistoryWritten;
		if (!target.HasHistory)
		{
			// The shader doesn't read the history in this frame, but its descriptor must still be readable: the pass only clears it
			RecordClear(commandBuffer, target.History[1 - written]);
		}
		// RecordPass binds the pipeline; push constants only need its layout, and stay set through it
		vkCmdPushConstants(commandBuffer, m_TAAResolvePipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
		RecordPass(commandBuffer, target.History[written], m_TAAResolvePipeline, target.ResolveSet[written]);
		target.HistoryWritten = written;
		target.HasHistory = true;
		return;
	}
	if (mode != AntialiasingMode::SMAA)
	{
		return;
	}
	{
		EnvMapVulkanProfiler::Scope scope(commandBuffer, "SMAA Edges", "Antialiasing, first pass: finds the edges in the finished "
			"picture, from the color differences between neighboring pixels.");
		RecordPass(commandBuffer, target.Edges, m_EdgesPipeline, target.EdgesSet);
	}
	{
		EnvMapVulkanProfiler::Scope scope(commandBuffer, "SMAA Weights", "Antialiasing, second pass: for each edge pixel, how much of "
			"its neighbors it takes, from the length and the shape of its edge. The longer an edge, the more it costs.");
		RecordPass(commandBuffer, target.Weights, m_WeightsPipeline, target.WeightsSet);
	}
}

void EnvMapVulkanAntialiasing::RecordResolve(VkCommandBuffer commandBuffer, Target& target, AntialiasingMode mode, Output output)
{
	if (mode == AntialiasingMode::SMAA)
	{
		EnvMapVulkanProfiler::Scope scope(commandBuffer, "SMAA Blend", "Antialiasing, last pass: each pixel mixed with its neighbors "
			"by the weights.");
		const float view = (float)output;
		vkCmdPushConstants(commandBuffer, m_BlendPipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(view), &view);
		DrawQuad(commandBuffer, m_BlendPipeline, target.BlendSet);
	}
	else if (mode == AntialiasingMode::TAA)
	{
		EnvMapVulkanProfiler::Scope scope(commandBuffer, "TAA Sharpen", "Antialiasing over the frames, last pass: the result a little "
			"sharper, since averaging softens it, without going past the colors of the pixel's neighbors.");
		const float view = (float)output;
		vkCmdPushConstants(commandBuffer, m_TAASharpenPipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(view), &view);
		DrawQuad(commandBuffer, m_TAASharpenPipeline, target.SharpenSet[target.HistoryWritten]);
	}
	else if (mode == AntialiasingMode::FXAA)
	{
		EnvMapVulkanProfiler::Scope scope(commandBuffer, "FXAA", "Antialiasing in one pass: each pixel on a contrast edge samples the "
			"picture a little across that edge. Cheaper and softer than SMAA.");
		DrawQuad(commandBuffer, m_FXAAPipeline, target.FXAASet);
	}
}
