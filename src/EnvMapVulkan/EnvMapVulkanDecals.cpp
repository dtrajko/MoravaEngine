#include "EnvMapVulkanDecals.h"

#include "EnvMapVulkanGBuffer.h"
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
#include <cstring>
#include <map>
#include <string>
#include <vector>


namespace EnvMapVulkanDecals
{
	// The std430 layout of Decal in Decals.glsl
	struct GPUDecal
	{
		glm::mat4 World;
		glm::mat4 Inverse;
		glm::vec4 Color;
		glm::vec4 Params;
	};
	static_assert(sizeof(GPUDecal) == 160, "layout mismatch with Decal in Decals.glsl");

	// The push constants of Decals.glsl
	struct DrawPush
	{
		glm::mat4 InverseViewProjection;
		glm::vec4 ViewSize;
	};
	static_assert(sizeof(DrawPush) == 80, "layout mismatch with DecalDrawPush in Decals.glsl");

	constexpr uint32_t FramesInFlight = H2M::VulkanFrameSync::MaxFramesInFlight;
	constexpr uint32_t DrawSet = 1, TextureSet = 2;

	struct Buffer
	{
		VkBuffer Handle = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		void* Mapped = nullptr;
		VkDeviceSize Size = 0;
	};

	struct TextureEntry
	{
		H2M::RefH2M<H2M::Texture2D_H2M> Texture;
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet Set;
		bool Failed = false;
	};

	// A run of the frame's decals with one texture: one instanced draw
	struct Group
	{
		std::string Texture;
		uint32_t First = 0, Count = 0;
	};

	struct State
	{
		VkPipelineLayout Layout = VK_NULL_HANDLE;
		VkPipeline Pipeline = VK_NULL_HANDLE;
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		VkFormat ColorFormat = VK_FORMAT_UNDEFINED, DepthFormat = VK_FORMAT_UNDEFINED;

		VkFramebuffer Framebuffer = VK_NULL_HANDLE;
		VkImageView ColorView = VK_NULL_HANDLE;
		VkImage FramebufferColor = VK_NULL_HANDLE;
		VkImageView FramebufferDepth = VK_NULL_HANDLE;
		uint32_t Width = 0, Height = 0;

		std::array<Buffer, FramesInFlight> Instances;   // host visible, one per frame in flight
		std::array<H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet, FramesInFlight> Sets;
		std::array<bool, FramesInFlight> SetsWritten = {};
		std::array<VkImageView, FramesInFlight> SetsDepth = {}, SetsNormals = {};

		std::vector<GPUDecal> FrameDecals;
		std::vector<Group> FrameGroups;

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
		return H2M::RendererH2M::GetShaderLibrary()->Get("Decals").As<H2M::VulkanShaderH2M>();
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
		H2M::VulkanAllocatorH2M allocator(std::string("Decals"));
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

	// The render pass: the scene's color (kept) and its depth, read-only (as the particles' render pass)
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
		// Before the passes after it (the particles, bloom, the composite)
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT |
			VK_ACCESS_TRANSFER_READ_BIT;

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

	static void CreatePipeline(VkFormat colorFormat, VkFormat depthFormat)
	{
		CreateRenderPass(colorFormat, depthFormat);
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = Shader();
		std::vector<VkDescriptorSetLayout> setLayouts = shader->GetAllDescriptorSetLayouts();
		VkPushConstantRange pushRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, (uint32_t)sizeof(DrawPush) };
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (uint32_t)setLayouts.size();
		layoutInfo.pSetLayouts = setLayouts.data();
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushRange;
		VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(Device(), &layoutInfo, nullptr, &s.Layout));

		const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();
		// No vertex buffers: the vertex stage makes the boxes from the vertex and instance index
		VkPipelineVertexInputStateCreateInfo vertexInput = {};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		// Both sides: the shader keeps the back faces (each pixel of the box once, also with the camera inside it)
		VkPipelineRasterizationStateCreateInfo rasterization = {};
		rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization.cullMode = VK_CULL_MODE_NONE;
		rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization.lineWidth = 1.0f;
		VkPipelineMultisampleStateCreateInfo multisample = {};
		multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		// No depth test: the shader reads the surface behind the box's back face itself
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;

		// Multiplied into the scene's color (the scene's alpha stays)
		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
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
		pipelineInfo.renderPass = s.RenderPass;
		pipelineInfo.subpass = 0;
		VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &s.Pipeline));

		for (uint32_t slot = 0; slot < FramesInFlight; slot++)
		{
			if (s.Sets[slot].DescriptorSets.empty())
			{
				s.Sets[slot] = shader->CreateDescriptorSets(DrawSet);
			}
		}
		if (!s.White)
		{
			const uint32_t white = 0xFFFFFFFF;
			s.White = H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::RGBA, 1, 1, &white);
		}
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

	static void DestroyPipeline()
	{
		if (s.Pipeline) vkDestroyPipeline(Device(), s.Pipeline, nullptr);
		if (s.Layout) vkDestroyPipelineLayout(Device(), s.Layout, nullptr);
		if (s.RenderPass) vkDestroyRenderPass(Device(), s.RenderPass, nullptr);
		s.Pipeline = VK_NULL_HANDLE;
		s.Layout = VK_NULL_HANDLE;
		s.RenderPass = VK_NULL_HANDLE;
		s.ColorFormat = s.DepthFormat = VK_FORMAT_UNDEFINED;
	}

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

	// ---- The textures' sets

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
				Log::GetLogger()->error("Decals: can't read the texture '{}' (a soft spot instead)", path);
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

	void Update(EnvMapVulkanScene& scene, uint32_t frameSlot)
	{
		s.FrameStats = Stats();
		s.FrameDecals.clear();
		s.FrameGroups.clear();

		// The decals with their textures, grouped by texture (the order in a group doesn't matter: multiplying)
		struct Item
		{
			const std::string* Texture;
			GPUDecal Decal;
		};
		std::vector<Item> items;
		auto add = [&items](const std::string& texture, const glm::mat4& world, const glm::vec4& color)
		{
			if (color.a <= 0.0f || std::abs(glm::determinant(glm::mat3(world))) < 1e-12f)
			{
				return; // nothing to see, or a flat box
			}
			Item item;
			item.Texture = &texture;
			item.Decal.World = world;
			item.Decal.Inverse = glm::inverse(world);
			item.Decal.Color = color;
			item.Decal.Params = glm::vec4(texture.empty() ? 0.0f : 1.0f, 0.0f, 0.0f, 0.0f);
			items.push_back(item);
		};
		scene.Each<DecalComponent>([&](EnvMapVulkanEntityID entity, DecalComponent& decal)
		{
			s.FrameStats.Placed++;
			add(decal.Settings.Texture, scene.GetWorldTransform(entity), decal.Settings.Color);
		});
		for (const SpawnedDecal& decal : scene.GetEffects().Decals)
		{
			s.FrameStats.Spawned++;
			glm::vec4 color = decal.Settings.Color;
			color.a *= decal.Opacity();
			add(decal.Settings.Texture, decal.Transform, color);
		}
		if (items.empty())
		{
			return;
		}
		std::stable_sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return *a.Texture < *b.Texture; });
		for (const Item& item : items)
		{
			if (s.FrameGroups.empty() || s.FrameGroups.back().Texture != *item.Texture)
			{
				Group group;
				group.Texture = *item.Texture;
				group.First = (uint32_t)s.FrameDecals.size();
				s.FrameGroups.push_back(group);
			}
			s.FrameGroups.back().Count++;
			s.FrameDecals.push_back(item.Decal);
		}
		s.FrameStats.Drawn = (uint32_t)s.FrameDecals.size();

		// Into the frame slot's buffer (grown when there are more)
		frameSlot %= FramesInFlight;
		Buffer& buffer = s.Instances[frameSlot];
		const VkDeviceSize size = s.FrameDecals.size() * sizeof(GPUDecal);
		if (buffer.Size < size)
		{
			if (buffer.Handle)
			{
				vkDeviceWaitIdle(Device());
				DestroyBuffer(buffer);
			}
			CreateBuffer(buffer, std::max<VkDeviceSize>(size * 2, 64 * sizeof(GPUDecal)));
			s.SetsWritten[frameSlot] = false;
		}
		memcpy(buffer.Mapped, s.FrameDecals.data(), (size_t)size);
	}

	// The frame slot's set: its decals, the depth, the normals
	static void WriteSet(uint32_t frameSlot, const EnvMapVulkanGBuffer& gbuffer)
	{
		const VkDescriptorBufferInfo decals = { s.Instances[frameSlot].Handle, 0, VK_WHOLE_SIZE };
		const VkDescriptorImageInfo depth = gbuffer.GetDepthInfo();
		const VkDescriptorImageInfo normals = gbuffer.GetNormalRoughnessInfo();
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = Shader();
		const VkDescriptorSet set = s.Sets[frameSlot].DescriptorSets[0];
		std::array<VkWriteDescriptorSet, 3> writes = { *shader->GetDescriptorSet("Decals", DrawSet), *shader->GetDescriptorSet("u_SceneDepth", DrawSet),
			*shader->GetDescriptorSet("u_SceneNormals", DrawSet) };
		for (VkWriteDescriptorSet& write : writes)
		{
			write.dstSet = set;
			write.descriptorCount = 1;
		}
		writes[0].pBufferInfo = &decals;
		writes[1].pImageInfo = &depth;
		writes[2].pImageInfo = &normals;
		vkUpdateDescriptorSets(Device(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
		s.SetsWritten[frameSlot] = true;
		s.SetsDepth[frameSlot] = depth.imageView;
		s.SetsNormals[frameSlot] = normals.imageView;
	}

	void Record(VkCommandBuffer commandBuffer, VkDescriptorSet frameSet, VkImage colorImage, VkFormat colorFormat, const EnvMapVulkanGBuffer& gbuffer,
		uint32_t width, uint32_t height, const glm::mat4& viewProjection, uint32_t frameSlot)
	{
		if (s.FrameDecals.empty() || !gbuffer.IsValid() || width == 0 || height == 0)
		{
			return;
		}
		frameSlot %= FramesInFlight;
		if (!s.RenderPass || s.ColorFormat != colorFormat || s.DepthFormat != gbuffer.GetDepthFormat())
		{
			if (s.RenderPass)
			{
				vkDeviceWaitIdle(Device());
				DestroyFramebuffer();
				DestroyPipeline();
			}
			CreatePipeline(colorFormat, gbuffer.GetDepthFormat());
		}
		UpdateFramebuffer(colorImage, gbuffer.GetDepthAttachmentView(), width, height);
		if (!s.SetsWritten[frameSlot] || s.SetsDepth[frameSlot] != gbuffer.GetDepthInfo().imageView ||
			s.SetsNormals[frameSlot] != gbuffer.GetNormalRoughnessInfo().imageView)
		{
			if (s.SetsWritten[frameSlot])
			{
				vkDeviceWaitIdle(Device()); // a set an earlier frame may still read
			}
			WriteSet(frameSlot, gbuffer);
		}

		EnvMapVulkanProfiler::Scope decalsScope(commandBuffer, "Decals", "The decals (bullet holes, stains): one instanced draw of boxes "
			"per texture; each pixel reads the scene's depth back to the world and, inside a box, multiplies the decal into the "
			"color.");

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
		push.InverseViewProjection = glm::inverse(viewProjection);
		push.ViewSize = glm::vec4((float)width, (float)height, 1.0f / (float)width, 1.0f / (float)height);
		const VkDescriptorSet sets[2] = { frameSet, s.Sets[frameSlot].DescriptorSets[0] };
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.Pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.Layout, H2M::VulkanShaderH2M::FrameDescriptorSet, 2, sets, 0, nullptr);
		vkCmdPushConstants(commandBuffer, s.Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
		for (const Group& group : s.FrameGroups)
		{
			VkDescriptorSet textureSet = GetTextureSet(group.Texture);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s.Layout, TextureSet, 1, &textureSet, 0, nullptr);
			vkCmdDraw(commandBuffer, 36, group.Count, 0, group.First);
			EnvMapVulkanProfiler::CountDraw(12 * group.Count);
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

	bool HasDecals()
	{
		return !s.FrameDecals.empty();
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
		DestroyPipeline();
		for (Buffer& buffer : s.Instances)
		{
			DestroyBuffer(buffer);
		}
		for (auto& set : s.Sets)
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
