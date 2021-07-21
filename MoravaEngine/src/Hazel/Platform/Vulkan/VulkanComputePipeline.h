#pragma once

#include "VulkanShader.h"
#include "VulkanTexture.h"

#include "vulkan/vulkan.h"

namespace Hazel {

	class VulkanComputePipeline : public RefCounted
	{
	public:
		VulkanComputePipeline(Ref<HazelShader> computeShader);
		~VulkanComputePipeline();

		void Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		void Begin();
		void Dispatch(VkDescriptorSet descriptorSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void End();

		Ref<VulkanShader> GetShader() { return m_Shader; }

		void SetPushConstants(const void* data, uint32_t size);
		void CreatePipeline();

	private:
		Ref<VulkanShader> m_Shader;

		VkPipelineLayout m_ComputePipelineLayout = nullptr;
		VkPipelineCache m_PipelineCache = nullptr;
		VkPipeline m_ComputePipeline = nullptr;

		VkCommandBuffer m_ActiveComputeCommandBuffer = nullptr;
	};

}
