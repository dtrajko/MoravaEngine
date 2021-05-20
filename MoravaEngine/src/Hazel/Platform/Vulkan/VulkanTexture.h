#pragma once

#include "Hazel/Renderer/HazelTexture.h"

#include "Vulkan.h"

#include "VulkanImage.h"


namespace Hazel {

	class VulkanTexture2D : public HazelTexture2D
	{
	public:
		VulkanTexture2D(const std::string& path, bool srgb = false, HazelTextureWrap wrap = HazelTextureWrap::Clamp);
		VulkanTexture2D(HazelImageFormat format, uint32_t width, uint32_t height, const void* data, HazelTextureWrap wrap = HazelTextureWrap::Clamp);
		VulkanTexture2D(HazelImageFormat format, uint32_t width, uint32_t height, HazelTextureWrap wrap = HazelTextureWrap::Clamp);
		virtual ~VulkanTexture2D();

		void Invalidate();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void Bind(uint32_t slot = 0) const override;

		virtual Ref<HazelImage2D> GetImage() const override { return m_Image; }
		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_DescriptorImageInfo; }

		void Lock() override;

		void Unlock() override;

		void Resize(uint32_t width, uint32_t height) override;

		Buffer GetWriteableBuffer() override;

		bool Loaded() const override;

		const std::string& GetPath() const override;

		HazelImageFormat GetFormat() const override;

		uint32_t GetMipLevelCount() const override;

		virtual uint64_t GetHash() const { return (uint64_t)m_Image; }
		RendererID GetRendererID() const override;

		bool operator ==(const HazelTexture& other) const override
		{
			throw std::logic_error("The method or operation is not implemented.");
		}

		virtual uint32_t GetID() const override { return uint32_t(); /* Not implemented */ }
	private:
		std::string m_Path;
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_Channels;

		Buffer m_ImageData;

		VkDeviceMemory m_DeviceMemory;
		VkImage m_Image;

		HazelImageFormat m_Format = HazelImageFormat::None;

		VkDescriptorImageInfo m_DescriptorImageInfo = {};
	};

	class VulkanTextureCube : public HazelTextureCube
	{
	public:
		VulkanTextureCube(HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		VulkanTextureCube(const std::string& path);
		virtual ~VulkanTextureCube();

		virtual const std::string& GetPath() const override { return ""; }

		virtual void Bind(uint32_t slot = 0) const override {}

		virtual HazelImageFormat GetFormat() const { return m_Format; }

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetMipLevelCount() const override;

		virtual bool operator==(const HazelTexture& other) const override { return false; }

		virtual uint64_t GetHash() const { return (uint64_t)m_Image; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_DescriptorImageInfo; }
		VkImageView CreateImageViewSingleMip(uint32_t mip);

		void GenerateMips(bool readonly = false);

		// abstract methods in HazelTexture
		virtual uint32_t GetID() const override { return uint32_t(); /* Not implemented */ }
		virtual uint32_t GetRendererID() const override { return uint32_t(); /* Not implemented */ }
		// virtual void Resize(uint32_t width, uint32_t height) override { /* Not implemented */ };

	private:
		void Invalidate();
	private:
		HazelImageFormat m_Format = HazelImageFormat::None;
		uint32_t m_Width = 0, m_Height = 0;

		bool m_MipsGenerated = false;

		Buffer m_LocalStorage;
		VkDeviceMemory m_DeviceMemory;
		VkImage m_Image;
		VkDescriptorImageInfo m_DescriptorImageInfo = {};
	};

}
