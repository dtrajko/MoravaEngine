#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelImage.h"

#include "vulkan/vulkan.h"


namespace Hazel {

	struct VulkanImageInfo
	{
		VkImage Image;
		VkImageView ImageView;
		VkSampler Sampler;
		VkDeviceMemory Memory;
	};

	class VulkanImage2D : public HazelImage2D
	{
	public:
		VulkanImage2D(HazelImageFormat format, uint32_t width, uint32_t height);
		virtual ~VulkanImage2D();

		virtual void Invalidate() override;
		virtual void Release() override;

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual HazelImageFormat GetFormat() const override { return m_Format; }

		VulkanImageInfo& GetImageInfo() { return m_Info; }
		const VulkanImageInfo& GetImageInfo() const { return m_Info; }

		const VkDescriptorImageInfo& GetDescriptor() { return m_DescriptorImageInfo; }

		virtual Buffer GetBuffer() const override { return m_ImageData; }
		virtual Buffer& GetBuffer() override { return m_ImageData; }

		virtual uint64_t GetHash() const override { return (uint64_t)m_Info.Image; }

		void UpdateDescriptor();
	private:
		HazelImageFormat m_Format;
		uint32_t m_Width = 0, m_Height = 0;

		Buffer m_ImageData;

		VulkanImageInfo m_Info;
		VkDescriptorImageInfo m_DescriptorImageInfo = {};
	};

	namespace Utils {

		inline VkFormat VulkanImageFormat(HazelImageFormat format)
		{
			switch (format)
			{
			case HazelImageFormat::RGBA:  return VK_FORMAT_R8G8B8A8_UNORM;
			case HazelImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			HZ_CORE_ASSERT(false);
			return VK_FORMAT_UNDEFINED;
		}

	}

}
