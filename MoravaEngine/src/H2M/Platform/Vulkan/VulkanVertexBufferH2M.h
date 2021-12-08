/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/VertexBufferH2M.h"

#include "VulkanAllocatorH2M.h"
#include "H2M/Core/BufferH2M.h"

#include "VulkanMemoryAllocator/vk_mem_alloc.h"


namespace H2M
{

	class VulkanVertexBufferH2M : public VertexBufferH2M
	{
	public:
		VulkanVertexBufferH2M(void* data, uint32_t size, VertexBufferUsageH2M usage = VertexBufferUsageH2M::Static);
		VulkanVertexBufferH2M(uint32_t size, VertexBufferUsageH2M usage = VertexBufferUsageH2M::Dynamic);
		virtual ~VulkanVertexBufferH2M() override;

		virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;
		virtual void RT_SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;
		virtual void Bind() const override {}

		virtual const VertexBufferLayoutH2M& GetLayout() const override { return VertexBufferLayoutH2M(); }
		virtual void SetLayout(const VertexBufferLayoutH2M& layout) override {}

		virtual unsigned int GetSize() const override { return m_Size; }
		virtual RendererID_H2M GetRendererID() const override { return 0; }

		VkBuffer GetVulkanBuffer() { return m_VulkanBuffer; }

	private:
		uint32_t m_Size = 0;
		BufferH2M m_LocalData;

		VkBuffer m_VulkanBuffer = nullptr;
		VkDeviceMemory m_DeviceMemory;
		VmaAllocation m_MemoryAllocation;

	};

}
