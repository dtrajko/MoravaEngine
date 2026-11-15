#include "EnvMapVulkanParticles.h"

#include "EnvMapVulkanGBuffer.h"
#include "EnvMapVulkanProfiler.h"
#include "EnvMapVulkanScene.h"

#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFrameSyncH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>


namespace EnvMapVulkanParticles
{
	// The std430 layouts of Include/Particles.glslh
	struct GPUParticle
	{
		glm::vec4 PositionAge;
		glm::vec4 VelocityLife;
		glm::vec4 Extra;
		glm::vec4 Light;
	};
	static_assert(sizeof(GPUParticle) == 64, "layout mismatch with Particle in Include/Particles.glslh");

	struct GPUEmitter
	{
		glm::vec4 ColorStart;
		glm::vec4 ColorEnd;
		glm::vec4 AccelerationDrag;
		glm::vec4 SizeSoftFlags;
		glm::vec4 Position;
		glm::vec4 PreviousPosition;
		glm::vec4 AxisCone;
		glm::vec4 VelocityInherit;
		glm::vec4 SpeedLife;
		glm::vec4 SpinRadius;
		glm::uvec4 Slice;
	};
	static_assert(sizeof(GPUEmitter) == 176, "layout mismatch with Emitter in Include/Particles.glslh");

	constexpr uint32_t FlagLit = 1, FlagAdditive = 2, FlagTextured = 4;

	// The push constants of ParticleUpdate.glsl, ParticleSpawn.glsl and Particles.glsl
	struct UpdatePush
	{
		float DeltaTime = 0.0f;
		uint32_t Count = 0;
	};
	struct LightPush
	{
		uint32_t Count = 0;
	};
	struct SpawnPush
	{
		float DeltaTime = 0.0f;
		uint32_t Emitter = 0;
		uint32_t Seed = 0;
		uint32_t Padding = 0;
	};
	struct DrawPush
	{
		glm::vec4 CameraRight;
		glm::vec4 CameraUp;
		glm::vec4 CameraForward;
		glm::vec4 DepthParams;
		glm::uvec4 Emitter;
	};
	static_assert(sizeof(DrawPush) == 80, "layout mismatch with ParticleDrawPush in Particles.glsl");

	constexpr uint32_t MaxPoolParticles = 1u << 20; // all emitters together
	constexpr uint32_t FramesInFlight = H2M::VulkanFrameSync::MaxFramesInFlight;
	constexpr uint32_t DrawSet = 1, TextureSet = 2;

	struct Buffer
	{
		VkBuffer Handle = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		void* Mapped = nullptr;
		VkDeviceSize Size = 0;
	};

	// An emitter's slice of the pool and what the CPU knows of it between frames
	struct Slot
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		uint32_t First = 0, Count = 0; // its particles in the pool
		uint32_t Head = 0;             // where the ring's next particle goes
		float Pending = 0.0f;          // a part of a particle owed from the last frames (rates don't come in whole numbers)
		glm::vec3 PreviousPosition = glm::vec3(0.0f);
		bool HasPrevious = false;
		std::deque<std::pair<double, uint32_t>> Started; // when particles started (the particles' clock) and how many: the count alive
		float MaxLifetime = 0.0f;
		bool ClearPending = false;     // a script's Clear: the slice zeroed in the next Record
		// This frame's
		uint32_t Spawn = 0;
		glm::vec3 Position = glm::vec3(0.0f);
		float Distance = 0.0f;         // from the camera, for the order they are drawn in
		std::string Texture;
		bool Additive = false;
	};

	struct TextureEntry
	{
		H2M::RefH2M<H2M::Texture2D_H2M> Texture;
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet Set;
		bool Failed = false;
	};

	struct State
	{
		bool Created = false;
		H2M::RefH2M<H2M::VulkanComputePipelineH2M> UpdatePipeline, SpawnPipeline, LightPipeline;
		std::array<H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet, FramesInFlight> UpdateSets, SpawnSets, DrawSets;
		// The light pass's: set 0 (a copy of the frame's lighting descriptors) and set 1 (the pool, the emitters)
		std::array<H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet, FramesInFlight> LightFrameSets, LightSets;
		std::array<VkDescriptorSet, FramesInFlight> LightFrameCopiedFrom = {}; // the frame set copied, and its update count then
		std::array<uint64_t, FramesInFlight> LightFrameCopiedAt = {};
		VkPipelineLayout DrawLayout = VK_NULL_HANDLE;
		VkPipeline AlphaPipeline = VK_NULL_HANDLE, AdditivePipeline = VK_NULL_HANDLE;
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		VkFormat ColorFormat = VK_FORMAT_UNDEFINED, DepthFormat = VK_FORMAT_UNDEFINED;

		VkFramebuffer Framebuffer = VK_NULL_HANDLE;
		VkImageView ColorView = VK_NULL_HANDLE;
		VkImage FramebufferColor = VK_NULL_HANDLE;
		VkImageView FramebufferDepth = VK_NULL_HANDLE;
		uint32_t Width = 0, Height = 0;

		Buffer Pool;                                     // device local
		bool PoolCleared = false;                        // zeroed since it was made (all dead)
		std::array<Buffer, FramesInFlight> Emitters;     // host visible, one per frame in flight
		std::array<bool, FramesInFlight> SetsWritten = {};  // the sets point at the current pool and emitter buffers
		std::array<VkImageView, FramesInFlight> SetsDepth = {}; // and at this depth view

		std::vector<Slot> Slots;
		std::unordered_map<EnvMapVulkanEntityID, uint32_t> SlotOf;
		uint32_t Capacity = 0;
		std::vector<GPUEmitter> FrameEmitters;
		float DeltaTime = 0.0f; // this frame's, on the particles' clock
		double Clock = 0.0;
		uint32_t Seed = 1;
		bool Warned = false;

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

	static void CreateBuffer(Buffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT_H2M(vkCreateBuffer(Device(), &bufferInfo, nullptr, &buffer.Handle));
		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(Device(), buffer.Handle, &requirements);
		H2M::VulkanAllocatorH2M allocator(std::string("Particles"));
		allocator.Allocate(requirements, &buffer.Memory, hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT_H2M(vkBindBufferMemory(Device(), buffer.Handle, buffer.Memory, 0));
		if (hostVisible)
		{
			VK_CHECK_RESULT_H2M(vkMapMemory(Device(), buffer.Memory, 0, VK_WHOLE_SIZE, 0, &buffer.Mapped));
		}
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

	static H2M::RefH2M<H2M::VulkanShaderH2M> DrawShader()
	{
		return H2M::RendererH2M::GetShaderLibrary()->Get("Particles").As<H2M::VulkanShaderH2M>();
	}

	// ---- The pipelines

	static void CreateComputePipelines()
	{
		s.UpdatePipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("ParticleUpdate"));
		s.SpawnPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("ParticleSpawn"));
		s.LightPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("ParticleLight"));
		for (uint32_t slot = 0; slot < FramesInFlight; slot++)
		{
			s.UpdateSets[slot] = s.UpdatePipeline->GetShader()->CreateDescriptorSets();
			s.SpawnSets[slot] = s.SpawnPipeline->GetShader()->CreateDescriptorSets();
			s.DrawSets[slot] = DrawShader()->CreateDescriptorSets(DrawSet);
			s.LightFrameSets[slot] = s.LightPipeline->GetShader()->CreateDescriptorSets(0);
			s.LightSets[slot] = s.LightPipeline->GetShader()->CreateDescriptorSets(1);
		}
		const uint32_t white = 0xFFFFFFFF;
		s.White = H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::RGBA, 1, 1, &white);
	}

	// The render pass: the scene's color (kept, left for the passes that sample it) and its depth, read-only
	static void CreateRenderPass(VkFormat colorFormat, VkFormat depthFormat)
	{
		std::array<VkAttachmentDescription, 2> attachments = {};
		attachments[0].format = colorFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // as the scene's render pass leaves it
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachments[1].format = depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // EnvMapVulkanGBuffer::MakeDepthReadable
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;
		subpass.pDepthStencilAttachment = &depthReference;

		std::array<VkSubpassDependency, 2> dependencies = {};
		// After the scene's passes wrote the color (and the depth became readable)
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
		// Before the passes after it read the color (bloom, the composite)
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = (uint32_t)attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();
		VK_CHECK_RESULT_H2M(vkCreateRenderPass(Device(), &renderPassInfo, nullptr, &s.RenderPass));
		s.ColorFormat = colorFormat;
		s.DepthFormat = depthFormat;
	}

	static VkPipeline CreateDrawPipeline(bool additive)
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = DrawShader();
		const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();

		// No vertex buffers: the vertex stage makes the squares from the vertex and instance index
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
		// Hidden behind the scene's surfaces, but not hiding each other: the depth is tested, never written
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		// Alpha: over the scene by the particle's alpha; additive: added (the shader multiplies by the alpha). The scene's
		// alpha stays
		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = additive ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachment.dstColorBlendFactor = additive ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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
		pipelineInfo.layout = s.DrawLayout;
		pipelineInfo.renderPass = s.RenderPass;
		pipelineInfo.subpass = 0;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
		return pipeline;
	}

	static void CreateDrawPipelines(VkFormat colorFormat, VkFormat depthFormat)
	{
		CreateRenderPass(colorFormat, depthFormat);
		// Set 0 (declared as the PBR shaders do: the per-frame set fits), set 1 (the pool, the emitters, the depth), set 2
		// (the texture), and the push constants of both stages
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = DrawShader();
		std::vector<VkDescriptorSetLayout> setLayouts = shader->GetAllDescriptorSetLayouts();
		VkPushConstantRange pushRange = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (uint32_t)sizeof(DrawPush) };
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (uint32_t)setLayouts.size();
		layoutInfo.pSetLayouts = setLayouts.data();
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushRange;
		VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(Device(), &layoutInfo, nullptr, &s.DrawLayout));
		s.AlphaPipeline = CreateDrawPipeline(false);
		s.AdditivePipeline = CreateDrawPipeline(true);
	}

	static void DestroyFramebuffer()
	{
		if (s.Framebuffer) vkDestroyFramebuffer(Device(), s.Framebuffer, nullptr);
		if (s.ColorView) vkDestroyImageView(Device(), s.ColorView, nullptr);
		s.Framebuffer = VK_NULL_HANDLE;
		s.ColorView = VK_NULL_HANDLE;
		s.FramebufferColor = VK_NULL_HANDLE;
		s.FramebufferDepth = VK_NULL_HANDLE;
	}

	static void DestroyDrawPipelines()
	{
		if (s.AlphaPipeline) vkDestroyPipeline(Device(), s.AlphaPipeline, nullptr);
		if (s.AdditivePipeline) vkDestroyPipeline(Device(), s.AdditivePipeline, nullptr);
		if (s.DrawLayout) vkDestroyPipelineLayout(Device(), s.DrawLayout, nullptr);
		if (s.RenderPass) vkDestroyRenderPass(Device(), s.RenderPass, nullptr);
		s.AlphaPipeline = s.AdditivePipeline = VK_NULL_HANDLE;
		s.DrawLayout = VK_NULL_HANDLE;
		s.RenderPass = VK_NULL_HANDLE;
		s.ColorFormat = s.DepthFormat = VK_FORMAT_UNDEFINED;
	}

	// The framebuffer of the scene's color image and the depth view (made again when either is new: a resize)
	static void UpdateFramebuffer(VkImage colorImage, VkImageView depthView, uint32_t width, uint32_t height)
	{
		if (s.Framebuffer && s.FramebufferColor == colorImage && s.FramebufferDepth == depthView && s.Width == width && s.Height == height)
		{
			return;
		}
		if (s.Framebuffer)
		{
			vkDeviceWaitIdle(Device()); // an earlier frame may still draw into the old one
		}
		DestroyFramebuffer();
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = colorImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = s.ColorFormat;
		viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		VK_CHECK_RESULT_H2M(vkCreateImageView(Device(), &viewInfo, nullptr, &s.ColorView));

		const VkImageView views[2] = { s.ColorView, depthView };
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = s.RenderPass;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = views;
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;
		VK_CHECK_RESULT_H2M(vkCreateFramebuffer(Device(), &framebufferInfo, nullptr, &s.Framebuffer));
		s.FramebufferColor = colorImage;
		s.FramebufferDepth = depthView;
		s.Width = width;
		s.Height = height;
	}

	// ---- The emitters' texture sets

	static VkDescriptorSet GetTextureSet(const std::string& path)
	{
		auto writeSet = [](H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set, const H2M::RefH2M<H2M::Texture2D_H2M>& texture)
		{
			set = DrawShader()->CreateDescriptorSets(TextureSet);
			VkWriteDescriptorSet write = *DrawShader()->GetDescriptorSet("u_Texture", TextureSet);
			write.dstSet = set.DescriptorSets[0];
			write.descriptorCount = 1;
			write.pImageInfo = &texture.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
			vkUpdateDescriptorSets(Device(), 1, &write, 0, nullptr);
		};
		if (path.empty())
		{
			if (s.WhiteSet.DescriptorSets.empty())
			{
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
				Log::GetLogger()->error("Particles: can't read the texture '{}' (a round puff instead)", path);
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

	// The slices again: a new emitter, one gone, or a changed MaxParticles. The particles alive go (a new pool)
	static void Relayout(const std::vector<std::pair<EnvMapVulkanEntityID, uint32_t>>& emitters)
	{
		std::unordered_map<EnvMapVulkanEntityID, Slot> previous;
		for (Slot& slot : s.Slots)
		{
			previous[slot.Entity] = std::move(slot);
		}
		s.Slots.clear();
		s.SlotOf.clear();
		uint32_t first = 0;
		for (const auto& [entity, count] : emitters)
		{
			Slot slot;
			if (auto it = previous.find(entity); it != previous.end())
			{
				slot = std::move(it->second);
			}
			slot.Entity = entity;
			slot.First = first;
			slot.Count = std::min(count, MaxPoolParticles - first);
			if (slot.Count < count && !s.Warned)
			{
				Log::GetLogger()->warn("Particles: more than {} particles in the scene's emitters: the last ones get fewer", MaxPoolParticles);
				s.Warned = true;
			}
			slot.Head = 0;
			slot.Started.clear();
			slot.ClearPending = false;
			first += slot.Count;
			s.SlotOf[entity] = (uint32_t)s.Slots.size();
			s.Slots.push_back(std::move(slot));
		}

		const uint32_t capacity = std::max(first, 1u);
		if (capacity != s.Capacity || !s.Pool.Handle)
		{
			if (s.Pool.Handle)
			{
				vkDeviceWaitIdle(Device()); // the frames in flight may still read the old pool
				DestroyBuffer(s.Pool);
			}
			CreateBuffer(s.Pool, (VkDeviceSize)capacity * sizeof(GPUParticle), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
			s.Capacity = capacity;
			s.SetsWritten.fill(false);
		}
		s.PoolCleared = false; // all dead: the slices hold other emitters' particles now
	}

	void Update(EnvMapVulkanScene& scene, float deltaTime, bool gameRunning, float timeScale, const std::vector<EnvMapVulkanEntityID>& selection,
		float rateScale, uint32_t frameSlot)
	{
		s.FrameStats = Stats();
		const float dt = std::max(deltaTime, 0.0f) * std::max(timeScale, 0.0f);
		s.DeltaTime = dt;
		s.Clock += dt;
		s.Seed = s.Seed * 1664525u + 1013904223u;

		// The emitters and their slices, in the order of their IDs (the same layout every frame while nothing changes)
		std::vector<std::pair<EnvMapVulkanEntityID, uint32_t>> emitters;
		scene.Each<ParticleEmitterComponent>([&](EnvMapVulkanEntityID entity, ParticleEmitterComponent& emitter)
		{
			const uint32_t count = (uint32_t)std::ceil((float)std::min(emitter.Settings.MaxParticles, ParticleEmitterSettings::MaxParticlesLimit) *
				glm::clamp(rateScale, 0.0f, 1.0f));
			emitters.emplace_back(entity, std::max(count, 1u));
		});
		std::sort(emitters.begin(), emitters.end());
		bool same = emitters.size() == s.Slots.size();
		for (size_t i = 0; same && i < emitters.size(); i++)
		{
			same = s.Slots[i].Entity == emitters[i].first && s.Slots[i].Count == std::min(emitters[i].second, MaxPoolParticles - s.Slots[i].First);
		}
		if (!same)
		{
			if (emitters.empty())
			{
				s.Slots.clear();
				s.SlotOf.clear();
			}
			else
			{
				Relayout(emitters);
			}
		}
		s.FrameStats.Emitters = (uint32_t)s.Slots.size();
		s.FrameStats.Capacity = s.Slots.empty() ? 0 : s.Capacity;
		if (s.Slots.empty())
		{
			return;
		}

		const std::unordered_set<EnvMapVulkanEntityID> selected(selection.begin(), selection.end());
		s.FrameEmitters.resize(s.Slots.size());
		for (uint32_t i = 0; i < (uint32_t)s.Slots.size(); i++)
		{
			Slot& slot = s.Slots[i];
			ParticleEmitterComponent& component = scene.Get<ParticleEmitterComponent>(slot.Entity);
			const ParticleEmitterSettings& p = component.Settings;
			const glm::mat4 world = scene.GetWorldTransform(slot.Entity);
			const glm::vec3 position = glm::vec3(world[3]);
			const glm::vec3 previous = slot.HasPrevious ? slot.PreviousPosition : position;
			glm::vec3 axis = glm::vec3(world[1]);
			axis = glm::length(axis) > 1e-6f ? glm::normalize(axis) : glm::vec3(0.0f, 1.0f, 0.0f);

			// How many start: the rate, the way it moved, a script's burst. A preview: the selected ones only
			const bool emits = gameRunning || IsPreviewed(scene, slot.Entity, selected);
			float count = 0.0f;
			if (emits && p.Emitting && dt > 0.0f)
			{
				count = (std::max(p.Rate, 0.0f) * dt + std::max(p.RatePerMeter, 0.0f) * glm::length(position - previous)) * glm::clamp(rateScale, 0.0f, 1.0f);
			}
			count += slot.Pending;
			uint32_t spawn = (uint32_t)count;
			slot.Pending = (emits && p.Emitting) ? count - (float)spawn : 0.0f;
			if (component.Burst > 0)
			{
				spawn += component.Burst;
				component.Burst = 0;
			}
			if (component.ClearRequested)
			{
				slot.ClearPending = true;
				slot.Started.clear();
				component.ClearRequested = false;
			}
			spawn = std::min(spawn, slot.Count);
			// Emitting: on with a rate (a slow rate starts nothing in most frames), or a burst now
			if ((emits && p.Emitting && (p.Rate > 0.0f || p.RatePerMeter > 0.0f)) || spawn > 0)
			{
				s.FrameStats.Emitting++;
			}
			if (spawn > 0)
			{
				s.FrameStats.Started += spawn;
				slot.Started.emplace_back(s.Clock, spawn);
			}
			// The count alive: those started within the longest lifetime (an upper bound), at most the slice
			slot.MaxLifetime = std::max(p.LifetimeMin, p.LifetimeMax);
			while (!slot.Started.empty() && slot.Started.front().first < s.Clock - slot.MaxLifetime)
			{
				slot.Started.pop_front();
			}
			uint32_t alive = 0;
			for (const auto& started : slot.Started)
			{
				alive += started.second;
			}
			s.FrameStats.Alive += std::min(alive, slot.Count);

			GPUEmitter& gpu = s.FrameEmitters[i];
			const ParticleEmitterSettings::BlendMode blend = p.Blend;
			uint32_t flags = (p.Lit ? FlagLit : 0) | (blend == ParticleEmitterSettings::BlendMode::Additive ? FlagAdditive : 0) |
				(p.Texture.empty() ? 0 : FlagTextured);
			gpu.ColorStart = p.ColorStart;
			gpu.ColorEnd = p.ColorEnd;
			gpu.AccelerationDrag = glm::vec4(p.Acceleration, std::max(p.Drag, 0.0f));
			gpu.SizeSoftFlags = glm::vec4(std::max(p.SizeStart, 0.0f), std::max(p.SizeEnd, 0.0f), std::max(p.SoftDistance, 0.0f), (float)flags);
			gpu.Position = glm::vec4(position, 0.0f);
			gpu.PreviousPosition = glm::vec4(previous, 0.0f);
			gpu.AxisCone = glm::vec4(axis, std::cos(glm::radians(glm::clamp(p.ConeAngle, 0.0f, 180.0f))));
			gpu.VelocityInherit = glm::vec4(dt > 0.0f ? (position - previous) / dt : glm::vec3(0.0f), p.InheritVelocity);
			gpu.SpeedLife = glm::vec4(p.SpeedMin, std::max(p.SpeedMax, p.SpeedMin), std::max(p.LifetimeMin, 0.001f), std::max(p.LifetimeMax, p.LifetimeMin));
			gpu.SpinRadius = glm::vec4(glm::radians(std::max(p.SpinSpeed, 0.0f)), std::max(p.StartRadius, 0.0f), 0.0f, 0.0f);
			gpu.Slice = glm::uvec4(slot.First, slot.Count, slot.Head, spawn);

			slot.Head = (slot.Head + spawn) % std::max(slot.Count, 1u);
			slot.Spawn = spawn;
			slot.Position = position;
			slot.Texture = p.Texture;
			slot.Additive = blend == ParticleEmitterSettings::BlendMode::Additive;
			slot.PreviousPosition = position;
			slot.HasPrevious = true;
		}

		// This frame's emitters into its buffer (grown when there are more)
		Buffer& buffer = s.Emitters[frameSlot % FramesInFlight];
		const VkDeviceSize size = s.FrameEmitters.size() * sizeof(GPUEmitter);
		if (buffer.Size < size)
		{
			if (buffer.Handle)
			{
				vkDeviceWaitIdle(Device());
				DestroyBuffer(buffer);
			}
			CreateBuffer(buffer, std::max<VkDeviceSize>(size * 2, 16 * sizeof(GPUEmitter)), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
			s.SetsWritten[frameSlot % FramesInFlight] = false;
		}
		memcpy(buffer.Mapped, s.FrameEmitters.data(), (size_t)size);
	}

	// The frame slot's sets: the pool, its emitter buffer, the depth
	static void WriteSets(uint32_t frameSlot, const VkDescriptorImageInfo& depthInfo)
	{
		const VkDescriptorBufferInfo pool = { s.Pool.Handle, 0, VK_WHOLE_SIZE };
		const VkDescriptorBufferInfo emitters = { s.Emitters[frameSlot].Handle, 0, VK_WHOLE_SIZE };
		std::vector<VkWriteDescriptorSet> writes;
		auto addBuffer = [&writes](const H2M::RefH2M<H2M::VulkanShaderH2M>& shader, VkDescriptorSet set, uint32_t setIndex, const char* name, const VkDescriptorBufferInfo* info)
		{
			VkWriteDescriptorSet write = *shader->GetDescriptorSet(name, setIndex);
			write.dstSet = set;
			write.descriptorCount = 1;
			write.pBufferInfo = info;
			writes.push_back(write);
		};
		addBuffer(s.UpdatePipeline->GetShader(), s.UpdateSets[frameSlot].DescriptorSets[0], 0, "ParticlePool", &pool);
		addBuffer(s.UpdatePipeline->GetShader(), s.UpdateSets[frameSlot].DescriptorSets[0], 0, "ParticleEmitters", &emitters);
		addBuffer(s.SpawnPipeline->GetShader(), s.SpawnSets[frameSlot].DescriptorSets[0], 0, "ParticlePool", &pool);
		addBuffer(s.SpawnPipeline->GetShader(), s.SpawnSets[frameSlot].DescriptorSets[0], 0, "ParticleEmitters", &emitters);
		addBuffer(s.LightPipeline->GetShader(), s.LightSets[frameSlot].DescriptorSets[0], 1, "ParticlePool", &pool);
		addBuffer(s.LightPipeline->GetShader(), s.LightSets[frameSlot].DescriptorSets[0], 1, "ParticleEmitters", &emitters);
		addBuffer(DrawShader(), s.DrawSets[frameSlot].DescriptorSets[0], DrawSet, "ParticlePool", &pool);
		addBuffer(DrawShader(), s.DrawSets[frameSlot].DescriptorSets[0], DrawSet, "ParticleEmitters", &emitters);
		VkWriteDescriptorSet depth = *DrawShader()->GetDescriptorSet("u_SceneDepth", DrawSet);
		depth.dstSet = s.DrawSets[frameSlot].DescriptorSets[0];
		depth.descriptorCount = 1;
		depth.pImageInfo = &depthInfo;
		writes.push_back(depth);
		vkUpdateDescriptorSets(Device(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
		s.SetsWritten[frameSlot] = true;
		s.SetsDepth[frameSlot] = depthInfo.imageView;
	}

	// The frame's lighting descriptors into the light pass's set 0 (its layout is the compute shader's own, so the frame's
	// set can't be bound): the bindings of Include/FrameSet.glslh but the camera's and the samplers' (the set has its own).
	// Again only when a descriptor was written since. Each frame slot has its set, last used by the frame two before, which
	// the GPU has finished: so the copy needs no wait, and goes around the sync shim (its form waits and counts, and the two
	// slots' copies would then set each other off every frame)
	static void CopyFrameSet(VkDescriptorSet frameSet, uint32_t frameSlot)
	{
		const uint64_t updates = H2M::VulkanFrameSync::GetDescriptorUpdateCount();
		if (s.LightFrameCopiedFrom[frameSlot] == frameSet && s.LightFrameCopiedAt[frameSlot] == updates)
		{
			return;
		}
		static constexpr uint32_t bindings[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 22, 23, 24, 25, 26, 27 };
		std::array<VkCopyDescriptorSet, sizeof(bindings) / sizeof(bindings[0])> copies = {};
		for (size_t i = 0; i < copies.size(); i++)
		{
			copies[i].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
			copies[i].srcSet = frameSet;
			copies[i].srcBinding = bindings[i];
			copies[i].dstSet = s.LightFrameSets[frameSlot].DescriptorSets[0];
			copies[i].dstBinding = bindings[i];
			copies[i].descriptorCount = 1;
		}
		(vkUpdateDescriptorSets)(Device(), 0, nullptr, (uint32_t)copies.size(), copies.data());
		s.LightFrameCopiedFrom[frameSlot] = frameSet;
		s.LightFrameCopiedAt[frameSlot] = H2M::VulkanFrameSync::GetDescriptorUpdateCount();
	}

	void Record(VkCommandBuffer commandBuffer, VkDescriptorSet frameSet, VkImage colorImage, VkFormat colorFormat, const EnvMapVulkanGBuffer& gbuffer,
		uint32_t width, uint32_t height, const glm::mat4& view, const glm::mat4& projection, uint32_t frameSlot)
	{
		if (s.Slots.empty() || !gbuffer.IsValid() || width == 0 || height == 0)
		{
			return;
		}
		frameSlot %= FramesInFlight;
		if (!s.Created)
		{
			CreateComputePipelines();
			s.Created = true;
		}
		if (!s.RenderPass || s.ColorFormat != colorFormat || s.DepthFormat != gbuffer.GetDepthFormat())
		{
			if (s.RenderPass)
			{
				vkDeviceWaitIdle(Device());
				DestroyFramebuffer();
				DestroyDrawPipelines();
			}
			CreateDrawPipelines(colorFormat, gbuffer.GetDepthFormat());
		}
		UpdateFramebuffer(colorImage, gbuffer.GetDepthAttachmentView(), width, height);
		const VkDescriptorImageInfo depthInfo = gbuffer.GetDepthInfo();
		if (!s.SetsWritten[frameSlot] || s.SetsDepth[frameSlot] != depthInfo.imageView)
		{
			if (s.SetsWritten[frameSlot])
			{
				vkDeviceWaitIdle(Device()); // a set an earlier frame may still read
			}
			WriteSets(frameSlot, depthInfo);
		}
		bool lit = false;
		for (const GPUEmitter& emitter : s.FrameEmitters)
		{
			lit |= ((uint32_t)(emitter.SizeSoftFlags.w + 0.5f) & FlagLit) != 0;
		}
		if (lit)
		{
			CopyFrameSet(frameSet, frameSlot);
		}

		EnvMapVulkanProfiler::Scope particlesScope(commandBuffer, "Particles", "The particle emitters' particles: two compute passes move "
			"and age every particle and start the frame's new ones (on the GPU only), a third lights each particle of the lit emitters "
		"once (the sun through its shadow, the sky), then one instanced draw per emitter of camera-facing "
			"squares, blended over the scene, tested against its depth without writing it, and faded where they meet a surface.");

		auto barrier = [commandBuffer](VkPipelineStageFlags sourceStages, VkAccessFlags sourceAccess, VkPipelineStageFlags destinationStages, VkAccessFlags destinationAccess)
		{
			VkMemoryBarrier memoryBarrier = {};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memoryBarrier.srcAccessMask = sourceAccess;
			memoryBarrier.dstAccessMask = destinationAccess;
			vkCmdPipelineBarrier(commandBuffer, sourceStages, destinationStages, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
		};
		const VkPipelineStageFlags compute = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		const VkAccessFlags readWrite = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

		// After the last frame's draws read the pool: zeroed where it must be (a new layout, a script's Clear)
		barrier(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | compute, readWrite, VK_PIPELINE_STAGE_TRANSFER_BIT | compute, VK_ACCESS_TRANSFER_WRITE_BIT | readWrite);
		bool filled = false;
		if (!s.PoolCleared)
		{
			vkCmdFillBuffer(commandBuffer, s.Pool.Handle, 0, VK_WHOLE_SIZE, 0);
			s.PoolCleared = true;
			filled = true;
		}
		for (Slot& slot : s.Slots)
		{
			if (slot.ClearPending)
			{
				vkCmdFillBuffer(commandBuffer, s.Pool.Handle, (VkDeviceSize)slot.First * sizeof(GPUParticle), (VkDeviceSize)slot.Count * sizeof(GPUParticle), 0);
				slot.ClearPending = false;
				filled = true;
			}
		}
		if (filled)
		{
			barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, compute, readWrite);
		}

		// Every particle a frame older, then the new ones
		if (s.DeltaTime > 0.0f)
		{
			UpdatePush push;
			push.DeltaTime = s.DeltaTime;
			push.Count = s.Capacity;
			VkPipelineLayout layout = s.UpdatePipeline->GetVulkanPipelineLayout();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, s.UpdatePipeline->GetVulkanPipeline());
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, s.UpdateSets[frameSlot].DescriptorSets.data(), 0, nullptr);
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
			vkCmdDispatch(commandBuffer, (s.Capacity + 63) / 64, 1, 1);
			barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, readWrite);
		}
		bool spawned = false;
		for (uint32_t i = 0; i < (uint32_t)s.Slots.size(); i++)
		{
			if (s.Slots[i].Spawn == 0)
			{
				continue;
			}
			if (!spawned)
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, s.SpawnPipeline->GetVulkanPipeline());
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, s.SpawnPipeline->GetVulkanPipelineLayout(), 0, 1,
					s.SpawnSets[frameSlot].DescriptorSets.data(), 0, nullptr);
				spawned = true;
			}
			SpawnPush push;
			push.DeltaTime = s.DeltaTime;
			push.Emitter = i;
			push.Seed = s.Seed ^ (i * 2654435761u);
			vkCmdPushConstants(commandBuffer, s.SpawnPipeline->GetVulkanPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
			vkCmdDispatch(commandBuffer, (s.Slots[i].Spawn + 63) / 64, 1, 1);
		}
		// The light of the lit emitters' particles: the old ones' and the new ones' (after both)
		if (lit)
		{
			barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, readWrite);
			LightPush push;
			push.Count = s.Capacity;
			VkPipelineLayout layout = s.LightPipeline->GetVulkanPipelineLayout();
			const VkDescriptorSet sets[2] = { s.LightFrameSets[frameSlot].DescriptorSets[0], s.LightSets[frameSlot].DescriptorSets[0] };
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, s.LightPipeline->GetVulkanPipeline());
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 2, sets, 0, nullptr);
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
			vkCmdDispatch(commandBuffer, (s.Capacity + 63) / 64, 1, 1);
		}
		barrier(compute | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT);

		// The draws: the emitters far to near
		const glm::mat4 cameraWorld = glm::inverse(view);
		const glm::vec3 cameraPosition = glm::vec3(cameraWorld[3]);
		std::vector<uint32_t> order;
		for (uint32_t i = 0; i < (uint32_t)s.Slots.size(); i++)
		{
			s.Slots[i].Distance = glm::length(s.Slots[i].Position - cameraPosition);
			order.push_back(i);
		}
		std::sort(order.begin(), order.end(), [](uint32_t a, uint32_t b) { return s.Slots[a].Distance > s.Slots[b].Distance; });

		VkRenderPassBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = s.RenderPass;
		beginInfo.framebuffer = s.Framebuffer;
		beginInfo.renderArea.extent = { width, height };
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		EnvMapVulkanProfiler::CountRenderPass(width, height);
		VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		VkRect2D scissor = { { 0, 0 }, { width, height } };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		DrawPush push;
		push.CameraRight = glm::vec4(view[0][0], view[1][0], view[2][0], 0.0f);
		push.CameraUp = glm::vec4(view[0][1], view[1][1], view[2][1], 0.0f);
		push.CameraForward = glm::vec4(-view[0][2], -view[1][2], -view[2][2], 0.0f);
		push.DepthParams = glm::vec4(projection[2][2], projection[3][2], projection[2][3], 0.0f);
		const VkDescriptorSet sets[2] = { frameSet, s.DrawSets[frameSlot].DescriptorSets[0] };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.DrawLayout, H2M::VulkanShaderH2M::FrameDescriptorSet, 2, sets, 0, nullptr);
		VkPipeline bound = VK_NULL_HANDLE;
		for (uint32_t i : order)
		{
			const Slot& slot = s.Slots[i];
			if (slot.Started.empty() && slot.Spawn == 0)
			{
				continue; // nothing alive in it
			}
			VkPipeline pipeline = slot.Additive ? s.AdditivePipeline : s.AlphaPipeline;
			if (pipeline != bound)
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				bound = pipeline;
			}
			VkDescriptorSet textureSet = GetTextureSet(slot.Texture);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.DrawLayout, TextureSet, 1, &textureSet, 0, nullptr);
			push.Emitter = glm::uvec4(i, 0, 0, 0);
			vkCmdPushConstants(commandBuffer, s.DrawLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
			vkCmdDraw(commandBuffer, 6, slot.Count, 0, slot.First);
			EnvMapVulkanProfiler::CountDraw(2 * slot.Count);
		}
		vkCmdEndRenderPass(commandBuffer);
	}

	void OnViewportResized()
	{
		if (!s.Framebuffer && !s.SetsWritten[0] && !s.SetsWritten[1 % FramesInFlight])
		{
			return;
		}
		vkDeviceWaitIdle(Device()); // the frames in flight may still draw with them
		DestroyFramebuffer();
		s.SetsWritten.fill(false);
	}

	bool HasEmitters()
	{
		return !s.Slots.empty();
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
		DestroyFramebuffer();
		DestroyDrawPipelines();
		DestroyBuffer(s.Pool);
		for (Buffer& buffer : s.Emitters)
		{
			DestroyBuffer(buffer);
		}
		for (uint32_t slot = 0; slot < FramesInFlight; slot++)
		{
			FreeSet(s.UpdateSets[slot]);
			FreeSet(s.SpawnSets[slot]);
			FreeSet(s.DrawSets[slot]);
			FreeSet(s.LightFrameSets[slot]);
			FreeSet(s.LightSets[slot]);
		}
		for (auto& [path, entry] : s.Textures)
		{
			FreeSet(entry.Set);
		}
		FreeSet(s.WhiteSet);
		s = State();
	}
}
