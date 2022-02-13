/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */
#include "VulkanUniformBufferH2M.h"

#include "H2M/Renderer/RendererH2M.h"


namespace H2M
{

	VulkanUniformBufferH2M::VulkanUniformBufferH2M(uint32_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{
		m_LocalStorage = new uint8_t[size];

		// RefH2M<VulkanUniformBufferH2M> instance = this;
		// RendererH2M::Submit([instance]() mutable {});

		{
			RT_Invalidate();
		}
	}

	VulkanUniformBufferH2M::~VulkanUniformBufferH2M()
	{
		Release();
	}

	void VulkanUniformBufferH2M::Release()
	{
		if (!m_MemoryAlloc)
		{
			return;
		}

		RendererH2M::SubmitResourceFree([buffer = m_Buffer, memoryAlloc = m_MemoryAlloc]()
		{
			VulkanAllocatorH2M allocator(std::string("UniformBuffer"));
			allocator.DestroyBuffer(buffer, memoryAlloc);
		});

		m_Buffer = nullptr;
		m_MemoryAlloc = nullptr;

		delete[] m_LocalStorage;
		m_LocalStorage = nullptr;
	}

	void VulkanUniformBufferH2M::SetData(const void* data, uint32_t size, uint32_t offset)
	{
	}

	void VulkanUniformBufferH2M::RT_SetData(const void* data, uint32_t size, uint32_t offset)
	{
	}

	void VulkanUniformBufferH2M::RT_Invalidate()
	{
	}

}
