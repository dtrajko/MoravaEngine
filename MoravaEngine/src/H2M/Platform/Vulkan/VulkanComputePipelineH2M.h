/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "VulkanShaderH2M.h"
#include "VulkanTextureH2M.h"

#include "vulkan/vulkan.h"


namespace H2M
{

	class VulkanComputePipelineH2M : public RefCountedH2M
	{
	public:
		VulkanComputePipelineH2M(RefH2M<ShaderH2M> computeShader);
		~VulkanComputePipelineH2M();

		void Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		void Begin();
		void Dispatch(VkDescriptorSet descriptorSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void End();

		RefH2M<VulkanShaderH2M> GetShader() { return m_Shader; }

		void SetPushConstants(const void* data, uint32_t size);
		void CreatePipeline();

	private:
		RefH2M<VulkanShaderH2M> m_Shader;

		VkPipelineLayout m_ComputePipelineLayout = nullptr;
		VkPipelineCache m_PipelineCache = nullptr;
		VkPipeline m_ComputePipeline = nullptr;

		VkCommandBuffer m_ActiveComputeCommandBuffer = nullptr;
	};

}
