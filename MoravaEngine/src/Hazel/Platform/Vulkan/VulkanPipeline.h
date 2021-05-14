#pragma once

#include "Hazel/Renderer/Pipeline.h"

#include "Vulkan.h"
#include "VulkanShader.h"

namespace Hazel {

	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineSpecification& spec);
		virtual ~VulkanPipeline();

		virtual PipelineSpecification& GetSpecification() { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const { return m_Specification; }

		virtual void Invalidate() override;

		virtual void Bind() override;

		VkPipeline GetVulkanPipeline() { return m_VulkanPipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() { return m_PipelineLayout; }
		VkDescriptorSet GetDescriptorSet() { return m_DescriptorSet.DescriptorSets[0]; }

	private:
		PipelineSpecification m_Specification;

		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_VulkanPipeline;
		VulkanShader::ShaderMaterialDescriptorSet m_DescriptorSet;
	};

}
