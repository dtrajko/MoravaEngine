#include "VulkanVertexBuffer.h"

#include "VulkanContext.h"

#include "Hazel/Renderer/HazelRenderer.h"

namespace Hazel {

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size, VertexBufferUsage usage)
		: m_Size(size)
	{
	}

	VulkanVertexBuffer::VulkanVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage)
		: m_Size(size)
	{
		m_LocalData = Buffer::Copy(data, size);

		//	Ref<VulkanVertexBuffer> instance = this;
		//	HazelRenderer::Submit([instance]() mutable
		//	{
		//		// TODO: Use staging
		//		auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		//	
		//		// Index buffer
		//		VkBufferCreateInfo indexbufferInfo = {};
		//		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		//		indexbufferInfo.size = instance->m_Size;
		//		indexbufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		//	
		//		// Copy index data to a buffer visible to the host
		//		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferInfo, nullptr, &instance->m_VulkanBuffer));
		//		VkMemoryRequirements memoryRequirements;
		//		vkGetBufferMemoryRequirements(device, instance->m_VulkanBuffer, &memoryRequirements);
		//	
		//		VulkanAllocator allocator(device, "VertexBuffer");
		//		allocator.Allocate(memoryRequirements, &instance->m_DeviceMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//	
		//		void* dstBuffer;
		//		VK_CHECK_RESULT(vkMapMemory(device, instance->m_DeviceMemory, 0, instance->m_Size, 0, &dstBuffer));
		//		memcpy(dstBuffer, instance->m_LocalData.Data, instance->m_Size);
		//		vkUnmapMemory(device, instance->m_DeviceMemory);
		//	
		//		VK_CHECK_RESULT(vkBindBufferMemory(device, instance->m_VulkanBuffer, instance->m_DeviceMemory, 0));
		//	});

		// TODO: Use staging
		auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		// Index buffer
		VkBufferCreateInfo indexbufferInfo = {};
		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferInfo.size = m_Size;
		indexbufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		// Copy index data to a buffer visible to the host
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferInfo, nullptr, &m_VulkanBuffer));
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, m_VulkanBuffer, &memoryRequirements);

		VulkanAllocator allocator(device, "VertexBuffer");
		allocator.Allocate(memoryRequirements, &m_DeviceMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* dstBuffer;
		VK_CHECK_RESULT(vkMapMemory(device, m_DeviceMemory, 0, m_Size, 0, &dstBuffer));
		memcpy(dstBuffer, m_LocalData.Data, m_Size);
		vkUnmapMemory(device, m_DeviceMemory);

		VK_CHECK_RESULT(vkBindBufferMemory(device, m_VulkanBuffer, m_DeviceMemory, 0));
	}

}
