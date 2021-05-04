#include "VulkanIndexBuffer.h"

#include "VulkanContext.h"

#include "Hazel/Renderer/HazelRenderer.h"

namespace Hazel {

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t size)
		: m_Size(size)
	{
	}

	VulkanIndexBuffer::VulkanIndexBuffer(void* data, uint32_t size)
		: m_Size(size)
	{
		m_LocalData = Buffer::Copy(data, size);

		Ref<VulkanIndexBuffer> instance = this;
		HazelRenderer::Submit([instance]() mutable
		{
			auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			// Index buffer
			VkBufferCreateInfo indexbufferInfo = {};
			indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			indexbufferInfo.size = instance->m_Size;
			indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

			// Copy index data to a buffer visible to the host
			VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferInfo, nullptr, &instance->m_VulkanBuffer));
			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(device, instance->m_VulkanBuffer, &memoryRequirements);

			VulkanAllocator allocator(device, "IndexBuffer");
			allocator.Allocate(memoryRequirements, &instance->m_DeviceMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			void* dstBuffer;
			VK_CHECK_RESULT(vkMapMemory(device, instance->m_DeviceMemory, 0, instance->m_Size, 0, &dstBuffer));
			memcpy(dstBuffer, instance->m_LocalData.Data, instance->m_Size);
			vkUnmapMemory(device, instance->m_DeviceMemory);

			VK_CHECK_RESULT(vkBindBufferMemory(device, instance->m_VulkanBuffer, instance->m_DeviceMemory, 0));
		});
	}

	void VulkanIndexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset)
	{
	}

	void VulkanIndexBuffer::Bind() const
	{
	}

	RendererID VulkanIndexBuffer::GetRendererID() const
	{
		return 0;
	}

}