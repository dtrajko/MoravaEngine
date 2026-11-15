#include "EnvMapVulkanTrails.h"

#include "EnvMapVulkanProfiler.h"
#include "EnvMapVulkanScene.h"

#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFrameSyncH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <unordered_set>


namespace EnvMapVulkanTrails
{
	// The std430 layout of TrailPoint in Trails.glsl
	struct GPUPoint
	{
		glm::vec4 PositionConnected; // xyz: on the surface, lifted; w: 1 joined to the point before
		glm::vec4 SideAlong;         // xyz: half the ribbon's width, sideways; w: meters along the ribbon
		glm::vec4 Color;
	};
	static_assert(sizeof(GPUPoint) == 48, "layout mismatch with TrailPoint in Trails.glsl");

	// The push constants of Trails.glsl
	struct DrawPush
	{
		glm::uvec4 Trail;   // x: its first point in the buffer; y: 1 textured
		glm::vec4 Texture;  // x: the meters along the ribbon a texture covers once
	};
	static_assert(sizeof(DrawPush) == 32, "layout mismatch with TrailDrawPush in Trails.glsl");

	constexpr uint32_t FramesInFlight = H2M::VulkanFrameSync::MaxFramesInFlight;
	constexpr uint32_t PointSet = 1, TextureSet = 2;

	struct Buffer
	{
		VkBuffer Handle = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		void* Mapped = nullptr;
		VkDeviceSize Size = 0;
	};

	// A trail's draw this frame
	struct Draw
	{
		uint32_t First = 0, Count = 0; // its points in the buffer
		std::string Texture;
		float TileLength = 1.0f;
	};

	struct TextureEntry
	{
		H2M::RefH2M<H2M::Texture2D_H2M> Texture;
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet Set;
		bool Failed = false;
	};

	struct State
	{
		VkPipelineLayout Layout = VK_NULL_HANDLE;
		VkPipeline Pipeline = VK_NULL_HANDLE;
		VkRenderPass PipelineRenderPass = VK_NULL_HANDLE; // the render pass the pipeline was made for

		std::array<Buffer, FramesInFlight> Points;       // host visible, one per frame in flight
		std::array<H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet, FramesInFlight> PointSets;
		std::array<VkBuffer, FramesInFlight> SetBuffer = {}; // the buffer each set points at

		std::vector<GPUPoint> FramePoints;
		std::vector<Draw> Draws;
		double Clock = 0.0;

		H2M::RefH2M<H2M::Texture2D_H2M> White;
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet WhiteSet;
		std::map<std::string, TextureEntry> Textures;

		Stats FrameStats;
	};
	static State s;

	static VkDevice Device()
	{
		return H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	}

	static H2M::RefH2M<H2M::VulkanShaderH2M> Shader()
	{
		return H2M::RendererH2M::GetShaderLibrary()->Get("Trails").As<H2M::VulkanShaderH2M>();
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
		H2M::VulkanAllocatorH2M allocator(std::string("Trails"));
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

	// ---- The pipeline

	static void DestroyPipeline()
	{
		if (s.Pipeline) vkDestroyPipeline(Device(), s.Pipeline, nullptr);
		if (s.Layout) vkDestroyPipelineLayout(Device(), s.Layout, nullptr);
		s.Pipeline = VK_NULL_HANDLE;
		s.Layout = VK_NULL_HANDLE;
		s.PipelineRenderPass = VK_NULL_HANDLE;
	}

	static void CreatePipeline(VkRenderPass renderPass)
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = Shader();
		// Set 0 (declared as the PBR shaders do: the per-frame set fits), set 1 (the points), set 2 (the texture)
		std::vector<VkDescriptorSetLayout> setLayouts = shader->GetAllDescriptorSetLayouts();
		VkPushConstantRange pushRange = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (uint32_t)sizeof(DrawPush) };
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (uint32_t)setLayouts.size();
		layoutInfo.pSetLayouts = setLayouts.data();
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushRange;
		VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(Device(), &layoutInfo, nullptr, &s.Layout));

		const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();
		// No vertex buffers: the vertex stage makes the ribbon from the points
		VkPipelineVertexInputStateCreateInfo vertexInput = {};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		// Seen from either side; pulled toward the camera a little (on top of the lift) so the road doesn't show through
		VkPipelineRasterizationStateCreateInfo rasterization = {};
		rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization.cullMode = VK_CULL_MODE_NONE;
		rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization.lineWidth = 1.0f;
		rasterization.depthBiasEnable = VK_TRUE;
		rasterization.depthBiasConstantFactor = -1.0f;
		rasterization.depthBiasSlopeFactor = -1.0f;
		VkPipelineMultisampleStateCreateInfo multisample = {};
		multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		// Multiplied: the scene's color times the shader's (1 where there's no mark). The scene's alpha stays
		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
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
		pipelineInfo.layout = s.Layout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &s.Pipeline));
		s.PipelineRenderPass = renderPass;
	}

	// ---- The trails' texture sets

	static VkDescriptorSet GetTextureSet(const std::string& path)
	{
		auto writeSet = [](H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set, const H2M::RefH2M<H2M::Texture2D_H2M>& texture)
		{
			set = Shader()->CreateDescriptorSets(TextureSet);
			VkWriteDescriptorSet write = *Shader()->GetDescriptorSet("u_Texture", TextureSet);
			write.dstSet = set.DescriptorSets[0];
			write.descriptorCount = 1;
			write.pImageInfo = &texture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
			vkUpdateDescriptorSets(Device(), 1, &write, 0, nullptr);
		};
		if (path.empty())
		{
			if (s.WhiteSet.DescriptorSets.empty())
			{
				if (!s.White)
				{
					const uint32_t white = 0xFFFFFFFF;
					s.White = H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::RGBA, 1, 1, &white);
				}
				writeSet(s.WhiteSet, s.White);
			}
			return s.WhiteSet.DescriptorSets[0];
		}
		TextureEntry& entry = s.Textures[path];
		if (entry.Set.DescriptorSets.empty() && !entry.Failed)
		{
			entry.Texture = ResourceManager::LoadTexture2D_H2M(path, true);
			if (!entry.Texture || !entry.Texture->Loaded())
			{
				Log::GetLogger()->error("Trails: can't read the texture '{}' (a plain band instead)", path);
				entry.Failed = true;
				entry.Texture = {};
			}
			else
			{
				writeSet(entry.Set, entry.Texture);
			}
		}
		return entry.Failed ? GetTextureSet("") : entry.Set.DescriptorSets[0];
	}

	// ---- Each frame

	static bool IsPreviewed(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const std::unordered_set<EnvMapVulkanEntityID>& selected)
	{
		for (EnvMapVulkanEntityID e = entity; e != NoEntity; e = scene.GetParent(e))
		{
			if (selected.count(e))
			{
				return true;
			}
		}
		return false;
	}

	// A trail's new points: one when it moved MinDistance from its last (or a new ribbon after a stop or a jump)
	static bool LeavePoints(TrailComponent& trail, const glm::vec3& position, const glm::vec3& up, bool emits)
	{
		const TrailSettings& p = trail.Settings;
		TrailPoints& points = trail.Points;
		if (!emits)
		{
			points.Open = false;
			return false;
		}
		TrailPoint point;
		point.Position = position;
		point.Up = up;
		point.Color = p.Color;
		point.Width = p.Width;
		point.Time = s.Clock;
		if (points.Open && points.GetCount() > 0)
		{
			const TrailPoint& last = points.Newest();
			const float distance = glm::length(position - last.Position);
			if (distance > TrailSettings::BreakDistance)
			{
				points.Push(point); // a jump: a new ribbon from here
				return true;
			}
			if (distance < std::max(p.MinDistance, 0.01f))
			{
				return true;
			}
			point.Along = last.Along + distance;
			point.Connected = true;
			points.Push(point);
			return true;
		}
		points.Push(point);
		points.Open = true;
		return true;
	}

	// A trail's points (and its head: where the entity is now, if it moved from its last point) into the frame's, each with
	// its ribbon's sideways half width (from the way the ribbon goes there: between the points before and after it) and
	// its color faded
	static void AddFramePoints(const TrailComponent& trail, const glm::vec3& position, const glm::vec3& up)
	{
		const TrailSettings& p = trail.Settings;
		const TrailPoints& points = trail.Points;
		std::vector<TrailPoint> ribbon;
		ribbon.reserve(points.GetCount() + 1);
		for (uint32_t i = 0; i < points.GetCount(); i++)
		{
			ribbon.push_back(points.Get(i));
		}
		if (points.Open && points.GetCount() > 0)
		{
			const TrailPoint& last = points.Newest();
			const float distance = glm::length(position - last.Position);
			if (distance > 0.005f)
			{
				TrailPoint head = last;
				head.Position = position;
				head.Up = up;
				head.Color = p.Color;
				head.Width = p.Width;
				head.Time = s.Clock;
				head.Along = last.Along + distance;
				head.Connected = true;
				ribbon.push_back(head);
			}
		}
		if (ribbon.size() < 2)
		{
			return;
		}
		ribbon[0].Connected = false; // the oldest left may have been joined to one gone

		Draw draw;
		draw.First = (uint32_t)s.FramePoints.size();
		draw.Count = (uint32_t)ribbon.size();
		draw.Texture = p.Texture;
		draw.TileLength = std::max(p.TileLength, 0.01f);
		const uint32_t capacity = points.GetCapacity();
		const bool full = points.GetCount() == capacity;
		const float fadeCount = std::max(1.0f, 0.1f * (float)capacity); // in a full ring the oldest tenth fades in
		glm::vec3 lastDirection = glm::vec3(0.0f, 0.0f, 1.0f);
		for (size_t i = 0; i < ribbon.size(); i++)
		{
			const TrailPoint& point = ribbon[i];
			const bool hasPrevious = i > 0 && point.Connected;
			const bool hasNext = i + 1 < ribbon.size() && ribbon[i + 1].Connected;
			glm::vec3 direction = (hasNext ? ribbon[i + 1].Position : point.Position) - (hasPrevious ? ribbon[i - 1].Position : point.Position);
			direction -= point.Up * glm::dot(direction, point.Up); // along the surface
			if (glm::length(direction) > 1e-5f)
			{
				lastDirection = glm::normalize(direction);
			}
			glm::vec3 side = glm::cross(point.Up, lastDirection);
			side = glm::length(side) > 1e-5f ? glm::normalize(side) : glm::vec3(1.0f, 0.0f, 0.0f);

			glm::vec4 color = point.Color;
			if (p.Lifetime > 0.0f)
			{
				const float age = (float)(s.Clock - point.Time);
				color.a *= glm::clamp(2.0f * (1.0f - age / p.Lifetime), 0.0f, 1.0f); // full over the first half, then fading
			}
			if (full)
			{
				color.a *= glm::clamp((float)i / fadeCount, 0.0f, 1.0f);
			}

			GPUPoint gpu;
			gpu.PositionConnected = glm::vec4(point.Position + point.Up * TrailSettings::Lift, point.Connected && i > 0 ? 1.0f : 0.0f);
			gpu.SideAlong = glm::vec4(side * (0.5f * point.Width), point.Along);
			gpu.Color = color;
			s.FramePoints.push_back(gpu);
		}
		s.Draws.push_back(std::move(draw));
	}

	void Update(EnvMapVulkanScene& scene, float deltaTime, bool gameRunning, float timeScale, const std::vector<EnvMapVulkanEntityID>& selection,
		uint32_t frameSlot)
	{
		s.FrameStats = Stats();
		s.FramePoints.clear();
		s.Draws.clear();
		s.Clock += (double)(std::max(deltaTime, 0.0f) * std::max(timeScale, 0.0f));

		const std::unordered_set<EnvMapVulkanEntityID> selected(selection.begin(), selection.end());
		scene.Each<TrailComponent>([&](EnvMapVulkanEntityID entity, TrailComponent& trail)
		{
			s.FrameStats.Trails++;
			TrailSettings& p = trail.Settings;
			p.Clamp();
			TrailPoints& points = trail.Points;
			points.SetCapacity(p.MaxPoints);
			if (trail.ClearRequested)
			{
				points.Clear();
				trail.ClearRequested = false;
			}
			// The marks past their lifetime go, oldest first
			while (p.Lifetime > 0.0f && points.GetCount() > 0 && s.Clock - points.Get(0).Time > (double)p.Lifetime)
			{
				points.PopOldest();
			}
			if (points.GetCount() == 0)
			{
				points.Open = false;
			}

			const glm::mat4 world = scene.GetWorldTransform(entity);
			const glm::vec3 position = glm::vec3(world[3]);
			glm::vec3 up = glm::vec3(world[1]);
			up = glm::length(up) > 1e-6f ? glm::normalize(up) : glm::vec3(0.0f, 1.0f, 0.0f);
			const bool emits = p.Emitting && (gameRunning || IsPreviewed(scene, entity, selected));
			if (LeavePoints(trail, position, up, emits))
			{
				s.FrameStats.Emitting++;
			}
			AddFramePoints(trail, position, up);
		});
		s.FrameStats.Points = (uint32_t)s.FramePoints.size();
		if (s.FramePoints.empty())
		{
			return;
		}

		// The frame's points into its buffer (grown when there are more)
		frameSlot %= FramesInFlight;
		Buffer& buffer = s.Points[frameSlot];
		const VkDeviceSize size = s.FramePoints.size() * sizeof(GPUPoint);
		if (buffer.Size < size)
		{
			if (buffer.Handle)
			{
				vkDeviceWaitIdle(Device()); // an earlier frame may still read it
				DestroyBuffer(buffer);
			}
			CreateBuffer(buffer, std::max<VkDeviceSize>(size * 2, 1024 * sizeof(GPUPoint)));
		}
		memcpy(buffer.Mapped, s.FramePoints.data(), (size_t)size);
	}

	void Record(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkDescriptorSet frameSet, uint32_t frameSlot)
	{
		if (s.Draws.empty() || renderPass == VK_NULL_HANDLE)
		{
			return;
		}
		frameSlot %= FramesInFlight;
		if (s.PipelineRenderPass != renderPass)
		{
			if (s.Pipeline)
			{
				vkDeviceWaitIdle(Device());
				DestroyPipeline();
			}
			CreatePipeline(renderPass);
		}
		// The frame slot's set points at its buffer (written again when the buffer grew: the slot's last frame is done
		// with it, Update waited)
		if (s.SetBuffer[frameSlot] != s.Points[frameSlot].Handle)
		{
			if (s.PointSets[frameSlot].DescriptorSets.empty())
			{
				s.PointSets[frameSlot] = Shader()->CreateDescriptorSets(PointSet);
			}
			const VkDescriptorBufferInfo info = { s.Points[frameSlot].Handle, 0, VK_WHOLE_SIZE };
			VkWriteDescriptorSet write = *Shader()->GetDescriptorSet("TrailPoints", PointSet);
			write.dstSet = s.PointSets[frameSlot].DescriptorSets[0];
			write.descriptorCount = 1;
			write.pBufferInfo = &info;
			vkUpdateDescriptorSets(Device(), 1, &write, 0, nullptr);
			s.SetBuffer[frameSlot] = s.Points[frameSlot].Handle;
		}

		EnvMapVulkanProfiler::Scope trailsScope(commandBuffer, "Trails", "The trails' marks (skid marks): one draw per trail, two "
			"triangles per piece of ribbon, made from its points in the vertex shader. Multiplied over the opaque meshes: the mark "
			"darkens the lit road under it. Tested against the depth, not written.");
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.Pipeline);
		const VkDescriptorSet sets[2] = { frameSet, s.PointSets[frameSlot].DescriptorSets[0] };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.Layout, H2M::VulkanShaderH2M::FrameDescriptorSet, 2, sets, 0, nullptr);
		for (const Draw& draw : s.Draws)
		{
			VkDescriptorSet textureSet = GetTextureSet(draw.Texture);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.Layout, TextureSet, 1, &textureSet, 0, nullptr);
			DrawPush push;
			push.Trail = glm::uvec4(draw.First, draw.Texture.empty() ? 0u : 1u, 0u, 0u);
			push.Texture = glm::vec4(draw.TileLength, 0.0f, 0.0f, 0.0f);
			vkCmdPushConstants(commandBuffer, s.Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
			vkCmdDraw(commandBuffer, (draw.Count - 1) * 6, 1, 0, 0);
			EnvMapVulkanProfiler::CountDraw(2 * (draw.Count - 1));
		}
	}

	bool HasPoints()
	{
		return !s.Draws.empty();
	}

	const Stats& GetStats()
	{
		return s.FrameStats;
	}

	void Shutdown()
	{
		if (!H2M::VulkanContextH2M::GetCurrentDevice())
		{
			return;
		}
		vkDeviceWaitIdle(Device());
		DestroyPipeline();
		for (Buffer& buffer : s.Points)
		{
			DestroyBuffer(buffer);
		}
		for (auto& set : s.PointSets)
		{
			FreeSet(set);
		}
		for (auto& [path, entry] : s.Textures)
		{
			FreeSet(entry.Set);
		}
		FreeSet(s.WhiteSet);
		s = State();
	}
}
