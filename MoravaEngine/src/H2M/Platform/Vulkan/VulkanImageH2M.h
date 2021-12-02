#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/ImageH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"

#include "vulkan/vulkan.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#include <map>


namespace H2M
{

	struct VulkanImageInfoH2M
	{
		VkDeviceMemory Memory;
		VkImage Image = nullptr;
		VkImageView ImageView = nullptr;
		VkSampler Sampler = nullptr;
		VmaAllocation MemoryAlloc = nullptr;
	};

	class VulkanImage2D_H2M : public Image2D_H2M
	{
	public:
		VulkanImage2D_H2M(ImageSpecificationH2M specification);
		VulkanImage2D_H2M(ImageFormatH2MH2M format, uint32_t width, uint32_t height);
		virtual ~VulkanImage2D_H2M() override;

		virtual void Invalidate() override;
		virtual void Release() override;

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual float GetAspectRatio() const override { return (float)m_Specification.Width / (float)m_Specification.Height; }

		virtual ImageSpecificationH2M& GetSpecification() override { return m_Specification; }
		virtual const ImageSpecificationH2M& GetSpecification() const override { return m_Specification; }

		void RT_Invalidate();

		virtual void CreatePerLayerImageViews() override;
		void RT_CreatePerLayerImageViews();
		void RT_CreatePerSpecificLayerImageViews(const std::vector<uint32_t>& layerIndices);

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

		virtual BufferH2M GetBuffer() const override { return m_ImageData; }
		virtual BufferH2M& GetBuffer() override { return m_ImageData; }

		virtual uint64_t GetHash() const override { return (uint64_t)m_Info.Image; }

		void UpdateDescriptor();

		// Debug
		static const std::map<VkImage, RefH2M<VulkanImage2D_H2M>>& GetImageRefs();

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

		VkFormat VulkanImageFormat(ImageFormatH2M format);

	}

}
