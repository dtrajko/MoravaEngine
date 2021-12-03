#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/PipelineH2M.h"

#include "VulkanH2M.h"


namespace H2M
{

	class VulkanPipelineH2M : public PipelineH2M
	{
	public:
		VulkanPipelineH2M(const PipelineSpecification& spec);
		virtual ~VulkanPipelineH2M();

		virtual PipelineSpecification& GetSpecification() { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const { return m_Specification; }

		virtual void Invalidate() override;

		virtual void Bind() override;

		VkPipeline GetVulkanPipeline() { return m_VulkanPipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() { return m_PipelineLayout; }

	private:
		PipelineSpecification m_Specification;

		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_VulkanPipeline;
		VulkanShaderH2M::ShaderMaterialDescriptorSet m_DescriptorSet;
	};

}
