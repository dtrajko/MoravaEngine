/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/ImageH2M.h"

#include "vulkan/vulkan.h"

#include <map>


namespace H2M
{

	struct VulkanImageInfoH2M
	{
		VkImage Image;
		VkImageView ImageView;
		VkSampler Sampler;
		VkDeviceMemory Memory;
	};

	class VulkanImage2D_H2M : public Image2D_H2M
	{
	public:
		VulkanImage2D_H2M(ImageSpecificationH2M specification);
		VulkanImage2D_H2M(ImageFormatH2M format, uint32_t width, uint32_t height);
		virtual ~VulkanImage2D_H2M();

		virtual void Invalidate() override;
		virtual void Release() override;

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual ImageFormatH2M GetFormat() const override { return m_Format; }
		virtual float GetAspectRatio() const override { return (float)m_Specification.Width / (float)m_Specification.Height; }

		virtual BufferH2M GetBuffer() const override { return m_ImageData; }
		virtual BufferH2M& GetBuffer() override { return m_ImageData; }

		virtual uint64_t GetHash() const override { return (uint64_t)m_Info.Image; }

		virtual VkImageView GetLayerImageView(uint32_t layer)
		{
			H2M_CORE_ASSERT(layer < m_PerLayerImageViews.size());
			return m_PerLayerImageViews[layer];
		}

		VkImageView GetMipImageView(uint32_t mip);
		VkImageView RT_GetMipImageView(uint32_t mip);

		VulkanImageInfoH2M& GetImageInfo() { return m_Info; }
		const VulkanImageInfoH2M& GetImageInfo() const { return m_Info; }

		const VkDescriptorImageInfo& GetDescriptor() { return m_DescriptorImageInfo; }

		void UpdateDescriptor();

	private:
		ImageSpecificationH2M m_Specification;

		ImageFormatH2M m_Format;
		uint32_t m_Width = 0, m_Height = 0;

		BufferH2M m_ImageData;

		VulkanImageInfoH2M m_Info;

		std::vector<VkImageView> m_PerLayerImageViews;
		std::map<uint32_t, VkImageView> m_MipImageViews;
		VkDescriptorImageInfo m_DescriptorImageInfo = {};

	};

	namespace Utils {

		inline VkFormat VulkanImageFormat(ImageFormatH2M format)
		{
			switch (format)
			{
			case ImageFormatH2M::RGBA:    return VK_FORMAT_R8G8B8A8_UNORM;
			case ImageFormatH2M::RGBA16F: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ImageFormatH2M::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			Log::GetLogger()->error("VulkanImageFormat: HazelImageFormat not supported: '{0}'!", format);
			// HZ_CORE_ASSERT(false);
			return VK_FORMAT_UNDEFINED;
		}

	}

}
