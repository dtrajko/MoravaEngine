#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelTexture.h"

#include "Vulkan.h"

#include "VulkanImage.h"


namespace Hazel {

	class VulkanTexture2D : public HazelTexture2D
	{
	public:
		VulkanTexture2D(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data, TextureProperties properties);
		VulkanTexture2D(const std::string& path, TextureProperties properties);
		// VulkanTexture2D(const std::string& path, bool srgb = false);
		// VulkanTexture2D(ImageFormatH2M format, uint32_t width, uint32_t height, TextureWrap wrap = TextureWrap::Clamp);
		virtual ~VulkanTexture2D();

		void Invalidate();

		ImageFormatH2M GetFormat() const override { return m_Format; };
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void Bind(uint32_t slot = 0) const override;

		virtual RefH2M<HazelImage2D> GetImage() const override { return m_Image; }

		void Lock() override;
		void Unlock() override;

		Buffer GetWriteableBuffer() override;

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual bool Loaded() const override;
		const std::string& GetPath() const override;
		uint32_t GetMipLevelCount() const override;

		void GenerateMips();

		virtual uint64_t GetHash() const { return (uint64_t)m_Image; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_DescriptorImageInfo; }

		// virtual RendererID GetRendererID() const override { return 0; } // Removed in Hazel Live 18.03.2021 #2

		bool operator ==(const HazelTexture& other) const override
		{
			throw std::logic_error("The method or operation is not implemented.");
		}

		virtual uint32_t GetID() const override { return uint32_t(0); /* Not implemented */ }

		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

	private:
		std::string m_Path;
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_Channels;
		TextureProperties m_Properties;

		Buffer m_ImageData;

		VkDeviceMemory m_DeviceMemory;
		VkImage m_VkImage;

		RefH2M<HazelImage2D> m_Image;

		ImageFormatH2M m_Format = ImageFormatH2M::None;

		VkDescriptorImageInfo m_DescriptorImageInfo = {};

		bool m_MipsGenerated = false;
	};

	class VulkanTextureCube : public HazelTextureCube
	{
	public:
		VulkanTextureCube(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data, TextureProperties properties);
		VulkanTextureCube(const std::string& path, TextureProperties properties);
		virtual ~VulkanTextureCube();

		virtual const std::string& GetPath() const override { return std::string(); }

		virtual void Bind(uint32_t slot = 0) const override {}

		virtual ImageFormatH2M GetFormat() const { return m_Format; }

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetMipLevelCount() const override;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

		virtual bool operator==(const HazelTexture& other) const override { return false; }

		virtual uint64_t GetHash() const { return (uint64_t)m_Image; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_DescriptorImageInfo; }

		VkImageView CreateImageViewSingleMip(uint32_t mip);

		void GenerateMips(bool readonly = false);

		// abstract methods in HazelTexture
		virtual uint32_t GetID() const override { return uint32_t(); /* Not implemented */ }

		// virtual RendererID GetRendererID() const override { return uint32_t(); // Removed in Hazel Live 18.03.2021 #2
		// virtual void Resize(uint32_t width, uint32_t height) override { /* Not implemented */ };

	private:
		void Invalidate();

	private:
		ImageFormatH2M m_Format = ImageFormatH2M::None;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		TextureProperties m_Properties;

		bool m_MipsGenerated = false;

		Buffer m_LocalStorage;
		VkDeviceMemory m_DeviceMemory;
		VkImage m_Image;
		VkDescriptorImageInfo m_DescriptorImageInfo = {};

	};

}
