/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */
#pragma once

#include "H2M/Renderer/UniformBufferH2M.h"

#include "VulkanAllocatorH2M.h"

#include "VulkanMemoryAllocator/vk_mem_alloc.h"


namespace H2M
{

	class VulkanUniformBufferH2M : public UniformBufferH2M
	{
	public:
		VulkanUniformBufferH2M(uint32_t size, uint32_t binding);
		virtual ~VulkanUniformBufferH2M();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void RT_SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual uint32_t GetBinding() const override { return m_Binding; }

		const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return m_DescriptorInfo; }

	private:
		void Release();
		void RT_Invalidate();

	private:
		VmaAllocation m_MemoryAlloc = nullptr;
		VkBuffer m_Buffer;
		VkDescriptorBufferInfo m_DescriptorInfo{};
		uint32_t m_Size = 0;
		uint32_t m_Binding = 0;
		std::string m_Name;
		VkShaderStageFlagBits m_ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

		uint8_t* m_LocalStorage = nullptr;
	};

}
