/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

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
		VulkanPipelineH2M(const PipelineSpecificationH2M& spec);
		virtual ~VulkanPipelineH2M();

		virtual PipelineSpecificationH2M& GetSpecification() { return m_Specification; }
		virtual const PipelineSpecificationH2M& GetSpecification() const { return m_Specification; }

		virtual void Invalidate() override;

		virtual void Bind() override;

		VkPipeline GetVulkanPipeline() { return m_VulkanPipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() { return m_PipelineLayout; }

	private:
		PipelineSpecificationH2M m_Specification;

		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_VulkanPipeline;
		VulkanShaderH2M::ShaderMaterialDescriptorSet m_DescriptorSet;
	};

}
