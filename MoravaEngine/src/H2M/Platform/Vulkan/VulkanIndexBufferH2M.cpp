/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanIndexBufferH2M.h"

#include "VulkanContextH2M.h"

#include "H2M/Renderer/RendererH2M.h"


namespace H2M
{

	VulkanIndexBufferH2M::VulkanIndexBufferH2M(uint32_t size)
		: m_Size(size)
	{
	}

	VulkanIndexBufferH2M::VulkanIndexBufferH2M(void* data, uint32_t size)
		: m_Size(size)
	{
		m_LocalData = BufferH2M::Copy(data, size);

		//	Ref<VulkanIndexBufferH2M> instance = this;
		//	HazelRenderer::Submit([instance]() mutable
		//	{
		//		auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		//	
		//		// Index buffer
		//		VkBufferCreateInfo indexbufferInfo = {};
		//		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		//		indexbufferInfo.size = instance->m_Size;
		//		indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		//	
		//		// Copy index data to a buffer visible to the host
		//		VK_CHECK_RESULT_H2M(vkCreateBuffer(device, &indexbufferInfo, nullptr, &instance->m_VulkanBuffer));
		//		VkMemoryRequirements memoryRequirements;
		//		vkGetBufferMemoryRequirements(device, instance->m_VulkanBuffer, &memoryRequirements);
		//	
		//		VulkanAllocator allocator(device, "IndexBuffer");
		//		allocator.Allocate(memoryRequirements, &instance->m_DeviceMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//	
		//		void* dstBuffer;
		//		VK_CHECK_RESULT_H2M(vkMapMemory(device, instance->m_DeviceMemory, 0, instance->m_Size, 0, &dstBuffer));
		//		memcpy(dstBuffer, instance->m_LocalData.Data, instance->m_Size);
		//		vkUnmapMemory(device, instance->m_DeviceMemory);
		//	
		//		VK_CHECK_RESULT_H2M(vkBindBufferMemory(device, instance->m_VulkanBuffer, instance->m_DeviceMemory, 0));
		//	});

		auto device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

		// Index buffer
		VkBufferCreateInfo indexbufferInfo = {};
		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferInfo.size = m_Size;
		indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		// Copy index data to a buffer visible to the host
		VK_CHECK_RESULT_H2M(vkCreateBuffer(device, &indexbufferInfo, nullptr, &m_VulkanBuffer));
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, m_VulkanBuffer, &memoryRequirements);

		VulkanAllocatorH2M allocator(std::string("IndexBuffer"));
		allocator.Allocate(memoryRequirements, &m_DeviceMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* dstBuffer;
		VK_CHECK_RESULT_H2M(vkMapMemory(device, m_DeviceMemory, 0, m_Size, 0, &dstBuffer));
		memcpy(dstBuffer, m_LocalData.Data, m_Size);
		vkUnmapMemory(device, m_DeviceMemory);

		VK_CHECK_RESULT_H2M(vkBindBufferMemory(device, m_VulkanBuffer, m_DeviceMemory, 0));
	}

	void VulkanIndexBufferH2M::SetData(void* buffer, uint32_t size, uint32_t offset)
	{
	}

	void VulkanIndexBufferH2M::Bind() const
	{
	}

	RendererID_H2M VulkanIndexBufferH2M::GetRendererID() const
	{
		return 0;
	}

}
