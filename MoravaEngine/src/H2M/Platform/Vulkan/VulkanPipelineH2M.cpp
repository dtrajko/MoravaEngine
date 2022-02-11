/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanPipelineH2M.h"

#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/RendererH2M.h"


namespace H2M
{

	namespace Utils
	{
		static VkPrimitiveTopology GetVulkanTopology(PrimitiveTopologyH2M topology)
		{
			switch (topology)
			{
				case PrimitiveTopologyH2M::Points:			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
				case PrimitiveTopologyH2M::Lines:			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				case PrimitiveTopologyH2M::Triangles:		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				case PrimitiveTopologyH2M::LineStrip:		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
				case PrimitiveTopologyH2M::TriangleStrip:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				case PrimitiveTopologyH2M::TriangleFan:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
			}

			H2M_CORE_ASSERT(false, "Unknown toplogy");
			return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
		}
	}

	static VkFormat ShaderDataTypeToVulkanFormat(ShaderDataTypeH2M type)
	{
		switch (type)
		{
			case ShaderDataTypeH2M::Float:  return VK_FORMAT_R32_SFLOAT;
			case ShaderDataTypeH2M::Float2: return VK_FORMAT_R32G32_SFLOAT;
			case ShaderDataTypeH2M::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataTypeH2M::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		}
		H2M_CORE_ASSERT(false);
		return VK_FORMAT_UNDEFINED;
	}

	VulkanPipelineH2M::VulkanPipelineH2M(const PipelineSpecificationH2M& spec)
		: m_Specification(spec)
	{
		// HZ_CORE_ASSERT(spec.Shader);
		if (!spec.Shader)
		{
			Log::GetLogger()->warn("VulkanPipelineH2M::VulkanPipelineH2M: spec.Shader is undefined!");
			return;
		}

		// HZ_CORE_ASSERT(spec.RenderPass);
		if (!spec.RenderPass)
		{
			Log::GetLogger()->warn("VulkanPipelineH2M::VulkanPipelineH2M: spec.RenderPass is undefined!");
			return;
		}

#if defined(SCENE_ENV_MAP_VULKAN)
		Invalidate();
		// InvalidateEnvMapVulkan();
		RendererH2M::RegisterShaderDependency(spec.Shader, RefH2M<PipelineH2M>(this));
#else
		Invalidate();
#endif
	}

	VulkanPipelineH2M::~VulkanPipelineH2M()
	{
		// TODO: delete pipeline
	}

	void VulkanPipelineH2M::Invalidate()
	{
		//	Ref<VulkanPipeline> instance = this;
		//	HazelRenderer::Submit([instance]() mutable {});
		{
			VkDevice device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

			// H2M_CORE_ASSERT(m_Specification.Shader);
			if (!m_Specification.Shader)
			{
				Log::GetLogger()->warn("VulkanPipelineH2M::Invalidate: m_Specification.Shader is undefined!");
			}

			// Ref<VulkanShader> vulkanShader = Ref<VulkanShader>(m_Specification.Shader);
			RefH2M<VulkanShaderH2M> vulkanShader = m_Specification.Shader.As<VulkanShaderH2M>();
			RefH2M<VulkanFramebufferH2M> framebuffer = m_Specification.RenderPass->GetSpecification().TargetFramebuffer.As<VulkanFramebufferH2M>();

			// VkDescriptorSetLayout descriptorSetLayout = vulkanShader->GetDescriptorSetLayout(0);
			std::vector<VkDescriptorSetLayout> descriptorSetLayouts = vulkanShader->GetAllDescriptorSetLayouts();

			//////////////////////////////////////////////////////////////////////
			// Push Constants
			//////////////////////////////////////////////////////////////////////

			const auto& pushConstantRanges = vulkanShader->GetPushConstantRanges();

			// TODO: should come from shader
			std::vector<VkPushConstantRange> vulkanPushConstantRanges(pushConstantRanges.size());
			for (uint32_t i = 0; i < pushConstantRanges.size(); i++)
			{
				const auto& pushConstantRange = pushConstantRanges[i];
				auto& vulkanPushConstantRange = vulkanPushConstantRanges[i];

				vulkanPushConstantRange.stageFlags = pushConstantRange.ShaderStage;
				vulkanPushConstantRange.offset = pushConstantRange.Offset;
				vulkanPushConstantRange.size = pushConstantRange.Size;
			}

			// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
			// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
			pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pPipelineLayoutCreateInfo.pNext = nullptr;

			// Descriptor Set Layouts
			pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
			pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

			pPipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(vulkanPushConstantRanges.size());
			pPipelineLayoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();

			VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

			// Create the graphics pipeline used in this example
			// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
			// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
			// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

			VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

			// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
			pipelineCreateInfo.layout = m_PipelineLayout;

			// Renderpass this pipeline is attached to
			/**** BEGIN Non-composite ****
			pipelineCreateInfo.renderPass = VulkanContextH2M::Get()->GetSwapChain().GetRenderPass();
			/**** END Non-composite ****/
			/**** BEGIN Composite ****/
			pipelineCreateInfo.renderPass = framebuffer->GetRenderPass();
			/**** END Composite ****/

			// Change line width and raster polygon mode
			// pipelineCreateInfo.pRasterizationState = VK_POLYGON_MODE_POINT | VK_POLYGON_MODE_LINE | VK_POLYGON_MODE_FILL

			// Construct the differnent states making up the pipeline

			// Input assembly state describes how primitives are assembled
			// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
			inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			// Rasterization state
			VkPipelineRasterizationStateCreateInfo rasterizationState = {};
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
			// rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
			rasterizationState.cullMode = VK_CULL_MODE_NONE;
			rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizationState.depthClampEnable = VK_FALSE;
			rasterizationState.rasterizerDiscardEnable = VK_FALSE;
			rasterizationState.depthBiasEnable = VK_FALSE;
			rasterizationState.lineWidth = 1.0f;

			// Color blend state describes how blend factors are calculated (if used)
			// We need one blend attachment state per color attachment (even if blending is not used)
			VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
			blendAttachmentState[0].colorWriteMask = 0xf;
			blendAttachmentState[0].blendEnable = VK_TRUE;
			blendAttachmentState[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendAttachmentState[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachmentState[0].colorBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState[0].alphaBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blendAttachmentState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

			VkPipelineColorBlendStateCreateInfo colorBlendState = {};
			colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendState.attachmentCount = 1;
			colorBlendState.pAttachments = blendAttachmentState;

			// Viewport state sets the number of viewports and scissor used in this pipeline
			// Note: This is actually overriden by the dynamic states (see below)
			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			// Enable dynamic states
			// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
			// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
			// For this example we will set the viewport and scissor using dynamic states
			std::vector<VkDynamicState> dynamicStateEnables;
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
			VkPipelineDynamicStateCreateInfo dynamicState = {};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.pDynamicStates = dynamicStateEnables.data();
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

			// Depth and stencil state containing depth and stencil compare and test operations
			// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
			VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
			depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilState.depthTestEnable = VK_TRUE;
			depthStencilState.depthWriteEnable = VK_TRUE;
			depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
			depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
			depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
			depthStencilState.stencilTestEnable = VK_FALSE;
			depthStencilState.front = depthStencilState.back;

			// Multi sampling state
			// This example does not make use fo multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
			VkPipelineMultisampleStateCreateInfo multisampleState = {};
			multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampleState.pSampleMask = nullptr;

			// Vertex input descriptions
			// Specifies the vertex input parameters for a pipeline

			// Vertex input descriptor

			VertexBufferLayoutH2M& layout = m_Specification.Layout;

			// Vertex input binding
			// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
			VkVertexInputBindingDescription vertexInputBinding = {};
			vertexInputBinding.binding = 0;
			/**** BEGIN Non-composite ****
			vertexInputBinding.stride = sizeof(Vertex);
			/**** END Non-composite ****/
			/**** BEGIN Composite ****/
			vertexInputBinding.stride = layout.GetStride();
			/**** END Composite ****/
			vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			// Input attribute bindings describe shader attribute locations and memory layouts
			// std::array<VkVertexInputAttributeDescription, 5> vertexInputAttributes;
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributes(layout.GetElementCount());

			/**** BEGIN Non-composite ****
			vertexInputAttributes[0].binding = 0;
			vertexInputAttributes[0].location = 0;
			vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexInputAttributes[0].offset = offsetof(Vertex, Position);

			vertexInputAttributes[1].binding = 0;
			vertexInputAttributes[1].location = 1;
			vertexInputAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexInputAttributes[1].offset = offsetof(Vertex, Normal);

			vertexInputAttributes[2].binding = 0;
			vertexInputAttributes[2].location = 2;
			vertexInputAttributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexInputAttributes[2].offset = offsetof(Vertex, Tangent);

			vertexInputAttributes[3].binding = 0;
			vertexInputAttributes[3].location = 3;
			vertexInputAttributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexInputAttributes[3].offset = offsetof(Vertex, Binormal);

			vertexInputAttributes[4].binding = 0;
			vertexInputAttributes[4].location = 4;
			vertexInputAttributes[4].format = VK_FORMAT_R32G32_SFLOAT;
			vertexInputAttributes[4].offset = offsetof(Vertex, Texcoord);
			/**** END Non-composite ****/

			/**** BEGIN Composite ****/
			uint32_t location = 0;
			for (auto element : layout)
			{
				vertexInputAttributes[location].binding = 0;
				vertexInputAttributes[location].location = location;
				vertexInputAttributes[location].format = ShaderDataTypeToVulkanFormat(element.Type);
				vertexInputAttributes[location].offset = element.Offset;

				location++;
			}
			/**** END Composite ****/

			// Vertex input state used for pipeline creation
			VkPipelineVertexInputStateCreateInfo vertexInputState = {};
			vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputState.vertexBindingDescriptionCount = 1;
			vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			const auto& shaderStages = vulkanShader->GetPipelineShaderStageCreateInfos(); // used in Vulkan Week
			// const auto& shaderStages = vulkanShader->GetPipelineShaderStageCreateInfos(); // used in more recent versions

			// Set pipeline shader stage info
			pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
			pipelineCreateInfo.pStages = shaderStages.data();

			// Assign the pipeline states to the pipeline creation info structure
			pipelineCreateInfo.pVertexInputState = &vertexInputState;
			pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
			pipelineCreateInfo.pRasterizationState = &rasterizationState;
			pipelineCreateInfo.pColorBlendState = &colorBlendState;
			pipelineCreateInfo.pMultisampleState = &multisampleState;
			pipelineCreateInfo.pViewportState = &viewportState;
			pipelineCreateInfo.pDepthStencilState = &depthStencilState;
			/**** BEGIN Non-composite ****/
			pipelineCreateInfo.renderPass = VulkanContextH2M::Get()->GetSwapChain().GetRenderPass();
			/**** END Non-composite ****/
			/**** BEGIN Composite ****
			pipelineCreateInfo.renderPass = framebuffer->GetRenderPass();
			/**** END Composite ****/
			pipelineCreateInfo.pDynamicState = &dynamicState;

			// What is this pipeline cache?
			VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
			pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			VkPipelineCache pipelineCache;
			VK_CHECK_RESULT_H2M(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

			// Create rendering pipeline using the specified states
			VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_VulkanPipeline));

			// Shader modules are no longer needed once the graphics pipeline has been created
			// vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
			// vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
		}
	}

	void VulkanPipelineH2M::InvalidateEnvMapVulkan()
	{
		// RefH2M<VulkanPipelineH2M> instance = this;
		// RendererH2M::Submit([instance]() mutable {});

		{
			// HZ_CORE_WARN("[VulkanPipeline] Creating pipeline {0}", instance->m_Specification.DebugName);

			VkDevice device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
			// HZ_CORE_ASSERT(instance->m_Specification.Shader);
			if (!m_Specification.Shader)
			{
				Log::GetLogger()->warn("VulkanPipelineH2M::Invalidate: m_Specification.Shader is undefined!");
			}

			RefH2M<VulkanShaderH2M> vulkanShader = RefH2M<VulkanShaderH2M>(m_Specification.Shader);
			RefH2M<VulkanFramebufferH2M> framebuffer = m_Specification.RenderPass->GetSpecification().TargetFramebuffer.As<VulkanFramebufferH2M>();

			auto descriptorSetLayouts = vulkanShader->GetAllDescriptorSetLayouts();

			const auto& pushConstantRanges = vulkanShader->GetPushConstantRanges();

			// TODO: should come from shader
			std::vector<VkPushConstantRange> vulkanPushConstantRanges(pushConstantRanges.size());
			for (uint32_t i = 0; i < pushConstantRanges.size(); i++)
			{
				const auto& pushConstantRange = pushConstantRanges[i];
				auto& vulkanPushConstantRange = vulkanPushConstantRanges[i];

				vulkanPushConstantRange.stageFlags = pushConstantRange.ShaderStage;
				vulkanPushConstantRange.offset = pushConstantRange.Offset;
				vulkanPushConstantRange.size = pushConstantRange.Size;
			}

			// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
			// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
			pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pPipelineLayoutCreateInfo.pNext = nullptr;
			pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
			pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
			pPipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)vulkanPushConstantRanges.size();
			pPipelineLayoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();

			VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

			// Create the graphics pipeline used in this example
			// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
			// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
			// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

			VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
			pipelineCreateInfo.layout = m_PipelineLayout;
			// Renderpass this pipeline is attached to
			pipelineCreateInfo.renderPass = framebuffer->GetRenderPass();

			// Construct the differnent states making up the pipeline

			// Input assembly state describes how primitives are assembled
			// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
			inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyState.topology = Utils::GetVulkanTopology(m_Specification.Topology);

			// Rasterization state
			VkPipelineRasterizationStateCreateInfo rasterizationState = {};
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.polygonMode = m_Specification.Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
			rasterizationState.cullMode = m_Specification.BackfaceCulling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
			rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizationState.depthClampEnable = VK_FALSE;
			rasterizationState.rasterizerDiscardEnable = VK_FALSE;
			rasterizationState.depthBiasEnable = VK_FALSE;
			rasterizationState.lineWidth = m_Specification.LineWidth; // this is dynamic

			// Color blend state describes how blend factors are calculated (if used)
			// We need one blend attachment state per color attachment (even if blending is not used)
			size_t colorAttachmentCount = framebuffer->GetSpecification().SwapChainTarget ? 1 : framebuffer->GetColorAttachmentCount();
			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates(colorAttachmentCount);
			if (framebuffer->GetSpecification().SwapChainTarget)
			{
				blendAttachmentStates[0].colorWriteMask = 0xf;
				blendAttachmentStates[0].blendEnable = VK_TRUE;
				blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
				blendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
				blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			}
			else
			{
				for (size_t i = 0; i < colorAttachmentCount; i++)
				{
					if (!framebuffer->GetSpecification().Blend)
						break;

					blendAttachmentStates[i].colorWriteMask = 0xf;
					if (!framebuffer->GetSpecification().Blend)
						break;

					const auto& attachmentSpec = framebuffer->GetSpecification().Attachments.Attachments[i];
					FramebufferBlendModeH2M blendMode = framebuffer->GetSpecification().BlendMode == FramebufferBlendModeH2M::None
						? attachmentSpec.BlendMode
						: framebuffer->GetSpecification().BlendMode;

					blendAttachmentStates[i].blendEnable = attachmentSpec.Blend ? VK_TRUE : VK_FALSE;
					if (blendMode == FramebufferBlendModeH2M::SrcAlphaOneMinusSrcAlpha)
					{
						blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
						blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
						blendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
						blendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					}
					else if (blendMode == FramebufferBlendModeH2M::OneZero)
					{
						blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
						blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
						blendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
						blendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
					}
					else if (blendMode == FramebufferBlendModeH2M::Zero_SrcColor)
					{
						blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
						blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
						blendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
						blendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
					}
					else
					{
						H2M_CORE_VERIFY(false);
					}
					blendAttachmentStates[i].colorBlendOp = VK_BLEND_OP_ADD;
					blendAttachmentStates[i].alphaBlendOp = VK_BLEND_OP_ADD;
				}
			}

			VkPipelineColorBlendStateCreateInfo colorBlendState = {};
			colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendState.attachmentCount = (uint32_t)blendAttachmentStates.size();
			colorBlendState.pAttachments = blendAttachmentStates.data();

			// Viewport state sets the number of viewports and scissor used in this pipeline
			// Note: This is actually overriden by the dynamic states (see below)
			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			// Enable dynamic states
			// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
			// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
			// For this example we will set the viewport and scissor using dynamic states
			std::vector<VkDynamicState> dynamicStateEnables;
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
			if (m_Specification.Topology == PrimitiveTopologyH2M::Lines || m_Specification.Topology == PrimitiveTopologyH2M::LineStrip || m_Specification.Wireframe)
				dynamicStateEnables.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);

			VkPipelineDynamicStateCreateInfo dynamicState = {};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.pDynamicStates = dynamicStateEnables.data();
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

			// Depth and stencil state containing depth and stencil compare and test operations
			// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
			VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
			depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilState.depthTestEnable = m_Specification.DepthTest ? VK_TRUE : VK_FALSE;
			depthStencilState.depthWriteEnable = m_Specification.DepthWrite ? VK_TRUE : VK_FALSE;
			depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
			depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
			depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
			depthStencilState.stencilTestEnable = VK_FALSE;
			depthStencilState.front = depthStencilState.back;

			// Multi sampling state
			// This example does not make use fo multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
			VkPipelineMultisampleStateCreateInfo multisampleState = {};
			multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampleState.pSampleMask = nullptr;

			// Vertex input descriptor
			VertexBufferLayoutH2M& layout = m_Specification.Layout;
			VertexBufferLayoutH2M& instanceLayout = m_Specification.InstanceLayout;

			std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;

			VkVertexInputBindingDescription& vertexInputBinding = vertexInputBindingDescriptions.emplace_back();
			vertexInputBinding.binding = 0;
			vertexInputBinding.stride = layout.GetStride();
			vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			if (instanceLayout.GetElementCount())
			{
				VkVertexInputBindingDescription& instanceInputBinding = vertexInputBindingDescriptions.emplace_back();
				instanceInputBinding.binding = 1;
				instanceInputBinding.stride = instanceLayout.GetStride();
				instanceInputBinding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
			}

			// Inpute attribute bindings describe shader attribute locations and memory layouts
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributes(layout.GetElementCount() + instanceLayout.GetElementCount());

			uint32_t location = 0;
			for (auto element : layout)
			{
				vertexInputAttributes[location].binding = 0;
				vertexInputAttributes[location].location = location;
				vertexInputAttributes[location].format = ShaderDataTypeToVulkanFormat(element.Type);
				vertexInputAttributes[location].offset = element.Offset;
				location++;
			}

			for (auto element : instanceLayout)
			{
				vertexInputAttributes[location].binding = 1;
				vertexInputAttributes[location].location = location;
				vertexInputAttributes[location].format = ShaderDataTypeToVulkanFormat(element.Type);
				vertexInputAttributes[location].offset = element.Offset;
				location++;
			}

			// Vertex input state used for pipeline creation
			VkPipelineVertexInputStateCreateInfo vertexInputState = {};
			vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputState.vertexBindingDescriptionCount = (uint32_t)vertexInputBindingDescriptions.size();
			vertexInputState.pVertexBindingDescriptions = vertexInputBindingDescriptions.data();
			vertexInputState.vertexAttributeDescriptionCount = (uint32_t)vertexInputAttributes.size();
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			const auto& shaderStages = vulkanShader->GetPipelineShaderStageCreateInfos();

			// Set pipeline shader stage info
			pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
			pipelineCreateInfo.pStages = shaderStages.data();

			// Assign the pipeline states to the pipeline creation info structure
			pipelineCreateInfo.pVertexInputState = &vertexInputState;
			pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
			pipelineCreateInfo.pRasterizationState = &rasterizationState;
			pipelineCreateInfo.pColorBlendState = &colorBlendState;
			pipelineCreateInfo.pMultisampleState = &multisampleState;
			pipelineCreateInfo.pViewportState = &viewportState;
			pipelineCreateInfo.pDepthStencilState = &depthStencilState;
			pipelineCreateInfo.renderPass = framebuffer->GetRenderPass();
			pipelineCreateInfo.pDynamicState = &dynamicState;

			// What is this pipeline cache?
			VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
			pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			VkPipelineCache pipelineCache;
			VK_CHECK_RESULT_H2M(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

			// Create rendering pipeline using the specified states
			VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_VulkanPipeline));

			// Shader modules are no longer needed once the graphics pipeline has been created
			// vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
			// vkDestroyShaderModule(device, shaderStages[1].module, nullptr);

			// instance->m_DescriptorSets = vulkanShader->AllocateDescriptorSets();

#if OLD
			const auto& shaderDescriptorSets = vulkanShader->GetShaderDescriptorSets();
			if (!shaderDescriptorSets.empty())
			{
				// Write default descriptor set... this overlaps materials somewhat, definitely requires more thought
				instance->m_DescriptorSet = vulkanShader->CreateDescriptorSets();
				std::vector<VkWriteDescriptorSet> writeDescriptors;

				for (auto&& [set, shaderDescriptorSet] : shaderDescriptorSets)
				{
					for (auto&& [binding, uniformBuffer] : shaderDescriptorSet.UniformBuffers)
					{
						VkWriteDescriptorSet writeDescriptorSet = {};
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writeDescriptorSet.pBufferInfo = &uniformBuffer->Descriptor;
						writeDescriptorSet.dstBinding = binding;
						writeDescriptorSet.dstSet = instance->m_DescriptorSet.DescriptorSets[0];
						writeDescriptors.push_back(writeDescriptorSet);
					}
				}

				HZ_CORE_WARN("VulkanPipeline - Updating {0} descriptor sets", writeDescriptors.size());
				vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
			}
#endif
		}
	}

	void VulkanPipelineH2M::Bind()
	{
	}

}
