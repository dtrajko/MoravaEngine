/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/IndexBufferH2M.h"

#include "H2M/Core/BufferH2M.h"

#include "VulkanAllocatorH2M.h"


namespace H2M
{

	class VulkanIndexBufferH2M : public IndexBufferH2M
	{
	public:
		VulkanIndexBufferH2M(uint32_t size);
		VulkanIndexBufferH2M(void* data, uint32_t size = 0);

		virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;
		virtual void Bind() const override;

		virtual uint32_t GetCount() const override { return m_Size / sizeof(uint32_t); }

		virtual uint32_t GetSize() const override { return m_Size; }
		virtual RendererID_H2M GetRendererID() const override;

		VkBuffer GetVulkanBuffer() { return m_VulkanBuffer; }
	private:
		uint32_t m_Size = 0;
		BufferH2M m_LocalData;

		VkBuffer m_VulkanBuffer;
		VkDeviceMemory m_DeviceMemory;
	};

}
