#include "EnvMapVulkanDebugDrawPass.h"

#include "EnvMapVulkanProfiler.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFrameSyncH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <deque>


static_assert(sizeof(EnvMapVulkanDebugDrawList::Triangle) == 48, "layout mismatch with DebugDraw.glsl");
static_assert(sizeof(EnvMapVulkanDebugDrawList::Line) == 32, "layout mismatch with DebugDraw.glsl");
static_assert(sizeof(EnvMapVulkanDebugDrawList::Dot) == 32, "layout mismatch with DebugDraw.glsl");

static float ColorBits(uint32_t color)
{
	float bits;
	memcpy(&bits, &color, sizeof(bits));
	return bits;
}

void EnvMapVulkanDebugDrawList::AddTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, uint32_t color)
{
	Triangles.push_back({ glm::vec4(a, ColorBits(color)), glm::vec4(b, 0.0f), glm::vec4(c, 0.0f) });
}

void EnvMapVulkanDebugDrawList::AddLine(const glm::vec3& a, const glm::vec3& b, uint32_t color, float width)
{
	Lines.push_back({ glm::vec4(a, ColorBits(color)), glm::vec4(b, width) });
}

void EnvMapVulkanDebugDrawList::AddDot(const glm::vec3& center, uint32_t color, float radius, uint32_t outline)
{
	Dots.push_back({ glm::vec4(center, ColorBits(color)), glm::vec4(radius, ColorBits(outline), 0.0f, 0.0f) });
}

void EnvMapVulkanDebugDrawList::Clear()
{
	Triangles.clear();
	Lines.clear();
	Dots.clear();
}

namespace EnvMapVulkanDebugDraw
{
	// The push constants of DebugDraw.glsl (128 bytes: the least every GPU has)
	struct DrawPush
	{
		glm::mat4 ViewProjection;
		glm::uvec4 Draw;     // x: the draw's first vec4 in its buffer, y: what it draws (0 triangles, 1 lines, 2 dots)
		glm::vec4 Viewport;  // xy: the size in pixels, z: the hidden parts' alpha, w: the depth tolerance (meters)
		glm::vec4 Camera;    // xyz: where it is, w: the draw distance (0: no limit)
		glm::vec4 Depth;     // xy: the projection's [2][2] and [3][2] (a depth to a distance)
	};
	static_assert(sizeof(DrawPush) == 128, "layout mismatch with DebugDrawPush in DebugDraw.glsl");

	constexpr uint32_t FramesInFlight = H2M::VulkanFrameSync::MaxFramesInFlight;
	constexpr uint32_t FrameSet = 0, PrimitiveSet = 1;
	constexpr VkDeviceSize PaletteSize = 64 * sizeof(uint32_t); // the frame buffer's first 256 bytes (16 vec4s)
	constexpr uint32_t PaletteVec4s = (uint32_t)(PaletteSize / sizeof(glm::vec4));

	struct Buffer
	{
		VkBuffer Handle = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		void* Mapped = nullptr;
		VkDeviceSize Size = 0;
	};

	// Where a list's primitives are in its buffer, in vec4s
	struct Layout
	{
		uint32_t Triangles = 0, TriangleCount = 0;
		uint32_t Lines = 0, LineCount = 0;
		uint32_t Dots = 0, DotCount = 0;
		uint32_t End = 0;
	};

	struct Persistent
	{
		uint64_t Key = 0;
		Buffer Data;
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet Set;
		Layout Where;
	};

	struct Retired
	{
		Persistent List;
		uint64_t Frame = 0; // the frame it was last drawn in
	};

	struct State
	{
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		VkPipeline Pipeline = VK_NULL_HANDLE;
		VkRenderPass PipelineRenderPass = VK_NULL_HANDLE;

		std::array<Buffer, FramesInFlight> FrameData; // the palette, then the frame's list
		std::array<H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet, FramesInFlight> FrameSets;     // depth + palette
		std::array<H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet, FramesInFlight> PrimitiveSets; // the frame's list
		std::array<VkBuffer, FramesInFlight> SetBuffer = {};
		std::array<VkImageView, FramesInFlight> SetDepth = {};

		EnvMapVulkanDebugDrawList FrameList;
		FrameSettings Settings;

		Persistent Current;
		bool CurrentSet = false; // set for the next frame
		std::deque<Retired> RetiredLists;
		EnvMapVulkanDebugDrawList Building; // reused: its memory stays

		uint64_t FrameCount = 0;
	};
	static State s;

	static VkDevice Device()
	{
		return H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	}

	static H2M::RefH2M<H2M::VulkanShaderH2M> Shader()
	{
		return H2M::RendererH2M::GetShaderLibrary()->Get("DebugDraw").As<H2M::VulkanShaderH2M>();
	}

	static void CreateBuffer(Buffer& buffer, VkDeviceSize size)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT_H2M(vkCreateBuffer(Device(), &bufferInfo, nullptr, &buffer.Handle));
		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(Device(), buffer.Handle, &requirements);
		H2M::VulkanAllocatorH2M allocator(std::string("DebugDraw"));
		allocator.Allocate(requirements, &buffer.Memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT_H2M(vkBindBufferMemory(Device(), buffer.Handle, buffer.Memory, 0));
		VK_CHECK_RESULT_H2M(vkMapMemory(Device(), buffer.Memory, 0, VK_WHOLE_SIZE, 0, &buffer.Mapped));
		buffer.Size = size;
	}

	static void DestroyBuffer(Buffer& buffer)
	{
		if (buffer.Mapped) vkUnmapMemory(Device(), buffer.Memory);
		if (buffer.Handle) vkDestroyBuffer(Device(), buffer.Handle, nullptr);
		if (buffer.Memory) vkFreeMemory(Device(), buffer.Memory, nullptr);
		buffer = Buffer();
	}

	static void FreeSet(H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set)
	{
		if (set.Pool)
		{
			vkDestroyDescriptorPool(Device(), set.Pool, nullptr);
		}
		set = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
	}

	static void DestroyPersistent(Persistent& list)
	{
		DestroyBuffer(list.Data);
		FreeSet(list.Set);
		list = Persistent();
	}

	static void WriteBufferBinding(VkDescriptorSet set, uint32_t setIndex, const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		const VkDescriptorBufferInfo info = { buffer, offset, range };
		VkWriteDescriptorSet write = *Shader()->GetDescriptorSet(name, setIndex);
		write.dstSet = set;
		write.descriptorCount = 1;
		write.pBufferInfo = &info;
		vkUpdateDescriptorSets(Device(), 1, &write, 0, nullptr);
	}

	// The vec4s a list takes, and where each kind starts after first
	static Layout GetLayout(const EnvMapVulkanDebugDrawList& list, uint32_t first)
	{
		Layout layout;
		layout.Triangles = first;
		layout.TriangleCount = (uint32_t)list.Triangles.size();
		layout.Lines = layout.Triangles + layout.TriangleCount * 3;
		layout.LineCount = (uint32_t)list.Lines.size();
		layout.Dots = layout.Lines + layout.LineCount * 2;
		layout.DotCount = (uint32_t)list.Dots.size();
		layout.End = layout.Dots + layout.DotCount * 2;
		return layout;
	}

	static void CopyList(const EnvMapVulkanDebugDrawList& list, const Layout& layout, void* mapped)
	{
		glm::vec4* data = (glm::vec4*)mapped;
		if (!list.Triangles.empty()) memcpy(data + layout.Triangles, list.Triangles.data(), list.Triangles.size() * sizeof(list.Triangles[0]));
		if (!list.Lines.empty()) memcpy(data + layout.Lines, list.Lines.data(), list.Lines.size() * sizeof(list.Lines[0]));
		if (!list.Dots.empty()) memcpy(data + layout.Dots, list.Dots.data(), list.Dots.size() * sizeof(list.Dots[0]));
	}

	// ---- The pipeline

	static void DestroyPipeline()
	{
		if (s.Pipeline) vkDestroyPipeline(Device(), s.Pipeline, nullptr);
		if (s.PipelineLayout) vkDestroyPipelineLayout(Device(), s.PipelineLayout, nullptr);
		s.Pipeline = VK_NULL_HANDLE;
		s.PipelineLayout = VK_NULL_HANDLE;
		s.PipelineRenderPass = VK_NULL_HANDLE;
	}

	static void CreatePipeline(VkRenderPass renderPass)
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = Shader();
		std::vector<VkDescriptorSetLayout> setLayouts = shader->GetAllDescriptorSetLayouts();
		VkPushConstantRange pushRange = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (uint32_t)sizeof(DrawPush) };
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (uint32_t)setLayouts.size();
		layoutInfo.pSetLayouts = setLayouts.data();
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushRange;
		VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(Device(), &layoutInfo, nullptr, &s.PipelineLayout));

		const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();
		// No vertex buffers: the vertex stage reads the primitives from the buffer
		VkPipelineVertexInputStateCreateInfo vertexInput = {};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		VkPipelineRasterizationStateCreateInfo rasterization = {};
		rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization.cullMode = VK_CULL_MODE_NONE;
		rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization.lineWidth = 1.0f;
		VkPipelineMultisampleStateCreateInfo multisample = {};
		multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		// The scene's depth is tested in the fragment stage (hidden parts are drawn faintly, not dropped): the overlay's
		// own depth attachment is neither tested nor written
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;

		// Premultiplied "over": the overlay keeps the color times its coverage, and the coverage, as the viewport composite
		// expects them (ViewportComposite.glsl)
		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo colorBlend = {};
		colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlend.attachmentCount = 1;
		colorBlend.pAttachments = &blendAttachment;

		std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = (uint32_t)stages.size();
		pipelineInfo.pStages = stages.data();
		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterization;
		pipelineInfo.pMultisampleState = &multisample;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlend;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = s.PipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &s.Pipeline));
		s.PipelineRenderPass = renderPass;
	}

	// ---- The lists

	EnvMapVulkanDebugDrawList& Frame()
	{
		return s.FrameList;
	}

	FrameSettings& Settings()
	{
		return s.Settings;
	}

	// The current persistent list to the retired ones (freed once no frame in flight reads it)
	static void RetireCurrent()
	{
		if (s.Current.Key != 0)
		{
			s.RetiredLists.push_back({ s.Current, s.FrameCount });
		}
		s.Current = Persistent();
	}

	void SetPersistent(uint64_t key, const std::function<void(EnvMapVulkanDebugDrawList&)>& fill)
	{
		if (key == 0)
		{
			return;
		}
		s.CurrentSet = true;
		if (key == s.Current.Key || !H2M::VulkanContextH2M::GetCurrentDevice())
		{
			return;
		}
		RetireCurrent();
		s.Building.Clear();
		fill(s.Building);
		s.Current.Key = key;
		s.Current.Where = GetLayout(s.Building, 0);
		if (s.Current.Where.End == 0)
		{
			return; // nothing to draw: no buffer
		}
		CreateBuffer(s.Current.Data, (VkDeviceSize)s.Current.Where.End * sizeof(glm::vec4));
		CopyList(s.Building, s.Current.Where, s.Current.Data.Mapped);
		s.Current.Set = Shader()->CreateDescriptorSets(PrimitiveSet);
		WriteBufferBinding(s.Current.Set.DescriptorSets[0], PrimitiveSet, "DebugPrimitives", s.Current.Data.Handle, 0, VK_WHOLE_SIZE);
		s.Building.Clear();
		s.Building.Triangles.shrink_to_fit(); // a large mesh's copy isn't kept
		s.Building.Lines.shrink_to_fit();
		s.Building.Dots.shrink_to_fit();
	}

	bool HasDrawings()
	{
		return !s.FrameList.IsEmpty() || (s.CurrentSet && s.Current.Where.End > 0);
	}

	// ---- Each frame

	void Record(VkCommandBuffer commandBuffer, const RecordInfo& info)
	{
		s.FrameCount++;
		// A persistent list nobody set for this frame goes; retired ones once no frame in flight can read them
		if (!s.CurrentSet)
		{
			RetireCurrent();
		}
		s.CurrentSet = false;
		while (!s.RetiredLists.empty() && s.RetiredLists.front().Frame + FramesInFlight < s.FrameCount)
		{
			DestroyPersistent(s.RetiredLists.front().List);
			s.RetiredLists.pop_front();
		}

		const bool persistent = s.Current.Where.End > 0;
		if ((s.FrameList.IsEmpty() && !persistent) || info.RenderPass == VK_NULL_HANDLE || info.SceneDepth.imageView == VK_NULL_HANDLE ||
			info.Width == 0 || info.Height == 0)
		{
			s.FrameList.Clear();
			return;
		}
		const uint32_t slot = info.FrameSlot % FramesInFlight;
		if (s.PipelineRenderPass != info.RenderPass)
		{
			if (s.Pipeline)
			{
				vkDeviceWaitIdle(Device());
				DestroyPipeline();
			}
			CreatePipeline(info.RenderPass);
		}

		// The palette and the frame's list into the slot's buffer (its last frame is done with it)
		const Layout frameLayout = GetLayout(s.FrameList, PaletteVec4s);
		Buffer& buffer = s.FrameData[slot];
		const VkDeviceSize size = (VkDeviceSize)frameLayout.End * sizeof(glm::vec4);
		if (buffer.Size < size)
		{
			if (buffer.Handle)
			{
				vkDeviceWaitIdle(Device()); // an earlier frame's set may still point at it
				DestroyBuffer(buffer);
			}
			CreateBuffer(buffer, std::max<VkDeviceSize>(size * 2, 4096 * sizeof(glm::vec4)));
		}
		memcpy(buffer.Mapped, s.Settings.Palette, PaletteSize);
		CopyList(s.FrameList, frameLayout, buffer.Mapped);

		if (s.SetBuffer[slot] != buffer.Handle || s.SetDepth[slot] != info.SceneDepth.imageView)
		{
			if (s.FrameSets[slot].DescriptorSets.empty())
			{
				s.FrameSets[slot] = Shader()->CreateDescriptorSets(FrameSet);
				s.PrimitiveSets[slot] = Shader()->CreateDescriptorSets(PrimitiveSet);
			}
			else if (s.SetDepth[slot] != info.SceneDepth.imageView)
			{
				vkDeviceWaitIdle(Device()); // a resize: the slot's set may still be read
			}
			VkWriteDescriptorSet depth = *Shader()->GetDescriptorSet("u_SceneDepth", FrameSet);
			depth.dstSet = s.FrameSets[slot].DescriptorSets[0];
			depth.descriptorCount = 1;
			depth.pImageInfo = &info.SceneDepth;
			vkUpdateDescriptorSets(Device(), 1, &depth, 0, nullptr);
			WriteBufferBinding(s.FrameSets[slot].DescriptorSets[0], FrameSet, "DebugPalette", buffer.Handle, 0, PaletteSize);
			WriteBufferBinding(s.PrimitiveSets[slot].DescriptorSets[0], PrimitiveSet, "DebugPrimitives", buffer.Handle, 0, VK_WHOLE_SIZE);
			s.SetBuffer[slot] = buffer.Handle;
			s.SetDepth[slot] = info.SceneDepth.imageView;
		}

		EnvMapVulkanProfiler::Scope debugScope(commandBuffer, "Debug Drawings", "The debug views' drawings in the world (the "
			"Navigation view's navmesh, paths and agents): triangles, and lines and dots made of two triangles each in the vertex "
			"shader, tested against the scene's depth in the fragment shader (what the scene hides is drawn faintly).");
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.Pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.PipelineLayout, FrameSet, 1, &s.FrameSets[slot].DescriptorSets[0], 0, nullptr);

		DrawPush push;
		push.ViewProjection = info.ViewProjection;
		push.Viewport = glm::vec4((float)info.Width, (float)info.Height, s.Settings.HiddenAlpha, s.Settings.DepthTolerance);
		push.Camera = glm::vec4(info.CameraPosition, std::max(s.Settings.DrawDistance, 0.0f));
		push.Depth = glm::vec4(info.Projection[2][2], info.Projection[3][2], 0.0f, 0.0f);
		auto drawList = [&](VkDescriptorSet primitives, const Layout& layout)
		{
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.PipelineLayout, PrimitiveSet, 1, &primitives, 0, nullptr);
			const struct { uint32_t First, Count, Kind, VerticesEach; } draws[3] = {
				{ layout.Triangles, layout.TriangleCount, 0, 3 }, { layout.Lines, layout.LineCount, 1, 6 }, { layout.Dots, layout.DotCount, 2, 6 } };
			for (const auto& draw : draws)
			{
				if (draw.Count == 0)
				{
					continue;
				}
				push.Draw = glm::uvec4(draw.First, draw.Kind, 0u, 0u);
				vkCmdPushConstants(commandBuffer, s.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
				vkCmdDraw(commandBuffer, draw.Count * draw.VerticesEach, 1, 0, 0);
				EnvMapVulkanProfiler::CountDraw(draw.Count * (draw.VerticesEach / 3));
			}
		};
		// The persistent list (the mesh) under the frame's (what moves on it)
		if (persistent)
		{
			drawList(s.Current.Set.DescriptorSets[0], s.Current.Where);
		}
		drawList(s.PrimitiveSets[slot].DescriptorSets[0], frameLayout);
		s.FrameList.Clear();
	}

	void Shutdown()
	{
		if (!H2M::VulkanContextH2M::GetCurrentDevice())
		{
			return;
		}
		vkDeviceWaitIdle(Device());
		DestroyPipeline();
		for (Buffer& buffer : s.FrameData)
		{
			DestroyBuffer(buffer);
		}
		for (auto& set : s.FrameSets)
		{
			FreeSet(set);
		}
		for (auto& set : s.PrimitiveSets)
		{
			FreeSet(set);
		}
		DestroyPersistent(s.Current);
		for (Retired& retired : s.RetiredLists)
		{
			DestroyPersistent(retired.List);
		}
		s = State();
	}
}
