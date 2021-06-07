#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Renderer/Pipeline.h"

#include "Vulkan.h"


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

	private:
		PipelineSpecification m_Specification;

		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_VulkanPipeline;
		VulkanShader::ShaderMaterialDescriptorSet m_DescriptorSet;
	};

}
