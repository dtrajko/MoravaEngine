#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/RefH2M.h"

#include "VulkanH2M.h"

#include <unordered_set>


namespace H2M {
	
	class VulkanPhysicalDeviceH2M : public RefCountedH2M
	{
	public:
		struct QueueFamilyIndicesH2M
		{
			int32_t Graphics = -1;
			int32_t Compute = -1;
			int32_t Transfer = -1;
		};
	public:
		VulkanPhysicalDeviceH2M();
		~VulkanPhysicalDeviceH2M();

		bool IsExtensionSupported(const std::string& extensionName) const;
		uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

		VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
		const QueueFamilyIndicesH2M& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

		const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }
		const VkPhysicalDeviceLimits& GetLimits() const { return m_Properties.limits; }
		const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_MemoryProperties; }

		VkFormat GetDepthFormat() const { return m_DepthFormat; }

		static RefH2M<VulkanPhysicalDeviceH2M> Select();

	private:
		VkFormat FindDepthFormat() const;
		QueueFamilyIndicesH2M GetQueueFamilyIndices(int queueFlags);

	private:
		QueueFamilyIndicesH2M m_QueueFamilyIndices;

		VkPhysicalDevice m_PhysicalDevice = nullptr;
		VkPhysicalDeviceProperties m_Properties;
		VkPhysicalDeviceFeatures m_Features;
		VkPhysicalDeviceMemoryProperties m_MemoryProperties;

		VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;

		std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
		std::unordered_set<std::string> m_SupportedExtensions;
		std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;

		friend class VulkanDeviceH2M;
	};

	// Represents a logical device
	class VulkanDeviceH2M : public RefCountedH2M
	{
	public:
		VulkanDeviceH2M(const RefH2M<VulkanPhysicalDeviceH2M>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);
		~VulkanDeviceH2M();

		VkQueue GetGraphicsQueue() { return m_Queue; } // the Graphics Queue
		VkQueue GetComputeQueue() { return m_ComputeQueue; }

		VkCommandBuffer GetCommandBuffer(bool begin, bool compute = false);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue);

		VkCommandBuffer CreateSecondaryCommandBuffer();

		const RefH2M<VulkanPhysicalDeviceH2M>& GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetVulkanDevice() const { return m_LogicalDevice; }

		/**** BEGIN Buffer Helper Functions (VulkanGameEngine by Brendan Galea) ****/

		void CreateBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory);

		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

		void CreateImageWithInfo(
			const VkImageCreateInfo& imageInfo,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& imageMemory);

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		/**** END Buffer Helper Functions (VulkanGameEngine by Brendan Galea) ****/

	private:
		VkDevice m_LogicalDevice = nullptr;
		RefH2M<VulkanPhysicalDeviceH2M> m_PhysicalDevice;
		VkPhysicalDeviceFeatures m_EnabledFeatures;
		VkCommandPool m_CommandPool, m_ComputeCommandPool;

		VkQueue m_Queue; // a.k.a. Graphics Queue (default)
		VkQueue m_ComputeQueue;

		bool m_EnableDebugMarkers = false;
	};

}
