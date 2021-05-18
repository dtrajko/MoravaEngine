#include "VulkanPipeline.h"

#include "VulkanShader.h"
#include "VulkanContext.h"
#include "Hazel/Renderer/HazelRenderer.h"


namespace Hazel {

	VulkanPipeline::VulkanPipeline(const PipelineSpecification& spec)
		: m_Specification(spec)
	{
		Invalidate();
	}

	VulkanPipeline::~VulkanPipeline()
	{
		// TODO: delete pipeline
	}

	void VulkanPipeline::Invalidate()
	{
		//	Ref<VulkanPipeline> instance = this;
		//	HazelRenderer::Submit([instance]() mutable
		//	{
		//	});

		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			HZ_CORE_ASSERT(m_Specification.Shader);
			Ref<VulkanShader> vulkanShader = Ref<VulkanShader>(m_Specification.Shader);

			VkDescriptorSetLayout descriptorSetLayout = vulkanShader->GetDescriptorSetLayout();

			// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
			// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
			pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pPipelineLayoutCreateInfo.pNext = nullptr;

			// Descriptor Set Layouts
			pPipelineLayoutCreateInfo.setLayoutCount = 1;
			pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

			//	const auto& pushConstantRanges = vulkanShader->GetPushConstantRanges();
			//	
			//	// TODO: should come from shader
			//	std::vector<VkPushConstantRange> vulkanPushConstantRanges(pushConstantRanges.size());
			//	for (uint32_t i = 0; i < pushConstantRanges.size(); i++)
			//	{
			//		const auto& pushConstantRange = pushConstantRanges[i];
			//		auto& vulkanPushConstantRange = vulkanPushConstantRanges[i];
			//	
			//		vulkanPushConstantRange.stageFlags = pushConstantRange.ShaderStage;
			//		vulkanPushConstantRange.offset = pushConstantRange.Offset;
			//		vulkanPushConstantRange.size = pushConstantRange.Size;
			//	}

			//////////////////////////////////////////////////////////////////////
			// Push Constants
			//////////////////////////////////////////////////////////////////////
			struct PushBlock
			{
				glm::mat4 Transform;
			};

			struct PushConstantRange
			{
				VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
				uint32_t Offset = 0;
				uint32_t Size = 0;
			};

			const std::vector<PushConstantRange> pushConstantRanges;
			std::vector<VkPushConstantRange> vulkanPushConstantRanges;

			PushConstantRange pushConstantRange;
			pushConstantRange.ShaderStage = VK_SHADER_STAGE_VERTEX_BIT;
			pushConstantRange.Offset = 0;
			pushConstantRange.Size = sizeof(PushBlock);

			VkPushConstantRange vulkanPushConstantRange;

			vulkanPushConstantRange.stageFlags = pushConstantRange.ShaderStage;
			vulkanPushConstantRange.offset = pushConstantRange.Offset;
			vulkanPushConstantRange.size = pushConstantRange.Size;

			vulkanPushConstantRanges.push_back(vulkanPushConstantRange);

			pPipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(vulkanPushConstantRanges.size());
			pPipelineLayoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();

			// pPipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(vulkanPushConstantRanges.size());
			// pPipelineLayoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();

			/**** BEGIN more advanced setup ****/
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
			/**** END more advanced setup ****/

			// Create the graphics pipeline used in this example
			// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
			// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
			// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

			VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)

			pipelineCreateInfo.layout = m_PipelineLayout;

			// Renderpass this pipeline is attached to
			pipelineCreateInfo.renderPass = VulkanContext::Get()->GetSwapChain().GetRenderPass();

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
			blendAttachmentState[0].blendEnable = VK_FALSE;
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

			// Vertex input binding
			// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
			VkVertexInputBindingDescription vertexInputBinding = {};
			vertexInputBinding.binding = 0;
			vertexInputBinding.stride = sizeof(Vertex);
			vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			// Inpute attribute bindings describe shader attribute locations and memory layouts
			std::array<VkVertexInputAttributeDescription, 5> vertexInputAttributes;

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

			// Vertex input state used for pipeline creation
			VkPipelineVertexInputStateCreateInfo vertexInputState = {};
			vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputState.vertexBindingDescriptionCount = 1;
			vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			const auto& shaderStages = vulkanShader->GetShaderStages(); // used in Vulkan Week
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
			pipelineCreateInfo.renderPass = VulkanContext::Get()->GetSwapChain().GetRenderPass();
			pipelineCreateInfo.pDynamicState = &dynamicState;

			// What is this pipeline cache?
			VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
			pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			VkPipelineCache pipelineCache;
			VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

			// Create rendering pipeline using the specified states
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &m_VulkanPipeline));

			// Shader modules are no longer needed once the graphics pipeline has been created
			// vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
			// vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
		}
	}

	void VulkanPipeline::Bind()
	{

	}

}
