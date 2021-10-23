#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Texture/MoravaTexture.h"

#include "Core/CommonValues.h"

#include "Hazel/Core/Buffer.h"
#include "Hazel/Renderer/HazelTexture.h"

#include <string>


class VulkanMoravaTexture : public MoravaTexture
{
public:
	VulkanMoravaTexture();
	// implementation from Hazel/Platform/Vulkan/VulkanTexture, VulkanTexture2D(const std::string & path, bool srgb = false, TextureWrap wrap = TextureWrap::Clamp);
	VulkanMoravaTexture(const char* fileLoc, bool flipVert = false, bool isSampler = false, int filter = 0);
	VulkanMoravaTexture(const char* fileLoc, uint32_t width, uint32_t height, bool isSampler, int filter);
	VulkanMoravaTexture(const char* fileLoc, Specification spec); // constructor for fully customizable texture
	virtual ~VulkanMoravaTexture();

	void Invalidate(); // from Hazel/Platform/Vulkan/VulkanTexture class
	virtual bool Load(bool flipVert = false);
	virtual void CreateAPISpecific();
	virtual void Save();
	virtual Hazel::HazelImageFormat GetFormat() { return m_Format; };

	// BEGIN pure virtual methods inherited from HazelTexture/HazelTexture2D
	virtual Hazel::Ref<Hazel::HazelImage2D> GetImage() const override { return Hazel::Ref<Hazel::HazelImage2D>(); }
	virtual void Lock() override {}
	virtual void Unlock() override {}
	virtual Hazel::Buffer GetWriteableBuffer() override { return Hazel::Buffer(); }
	virtual void Resize(uint32_t width, uint32_t height) override {}
	virtual bool Loaded() const override { return m_Buffer ? true : false; }
	virtual const std::string& GetPath() const override { return std::string(m_FileLocation); }
	virtual void Bind(uint32_t textureSlot = 0) const override;
	virtual Hazel::HazelImageFormat GetFormat() const override { return Hazel::HazelImageFormat(); }
	virtual uint32_t GetMipLevelCount() const override { return uint32_t(); }
	virtual uint64_t GetHash() const override { return uint64_t(); }
	virtual bool operator==(const Hazel::HazelTexture& other) const override { return m_ID == other.GetID(); }
	virtual Hazel::RendererID GetRendererID() const override { return m_ID; }
	// END pure virtual methods inherited from HazelTexture/HazelTexture2D

	virtual bool IsLoaded() const override { return m_Buffer ? true : false; }; // used in Hazel::Mesh

	inline uint32_t GetID() const { return m_ID; };
	inline uint32_t GetWidth() const { return m_Spec.Width; };
	inline uint32_t GetHeight() const { return m_Spec.Height; };

	virtual void Unbind() override;
	virtual void Clear() override;
	virtual uint32_t CalculateMipMapCount(uint32_t width, uint32_t height) override; // used in Hazel::SceneRenderer
	virtual uint32_t GetMipLevelCount() override;
	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

	// Getters
	virtual int GetRed(int x, int z) override;
	virtual int GetGreen(int x, int z) override;
	virtual int GetBlue(int x, int z) override;
	virtual int GetAlpha(int x, int z) override;
	virtual float GetFileSize(const char* filename) override;

	// Setters
	virtual void SetPixel(int x, int z, glm::ivec4 pixel) override;
	virtual void SetRed(int x, int z, int value) override;
	virtual void SetGreen(int x, int z, int value) override;
	virtual void SetBlue(int x, int z, int value) override;
	virtual void SetAlpha(int x, int z, int value) override;

	void GenerateMips(bool readonly = false);

protected:
	const char* m_FileLocation;
	unsigned char* m_Buffer;
	int m_Level;
	Hazel::HazelImageFormat m_Format;

	// from Hazel/Platform/Vulkan/VulkanTexture
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Channels;
	Hazel::Buffer m_ImageData;

	Hazel::Ref<Hazel::HazelImage2D> m_Image;
	VkImage m_VulkanImage;

	Hazel::TextureProperties m_Properties;
	VkDeviceMemory m_DeviceMemory;
	VkDescriptorImageInfo m_DescriptorImageInfo = {};

};
