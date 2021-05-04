#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Core/Buffer.h"
#include "Hazel/Renderer/HazelImage.h"
#include "Hazel/Asset/Asset.h"


namespace Hazel {

	enum class HazelTextureFormat
	{
		None = 0,
		RGB = 1,
		RGBA = 2,
		Float16 = 3
	};

	enum class HazelTextureWrap
	{
		None = 0,
		Clamp = 1,
		Repeat = 2
	};

	enum class HazelTextureType
	{
		None = 0,
		Texture2D,
		TextureCube
	};

	class HazelTexture : public Asset
	{
	public:
		virtual ~HazelTexture() {}

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual HazelImageFormat GetFormat() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;

		virtual uint64_t GetHash() const = 0;

		virtual HazelTextureType GetType() const = 0;

		virtual bool operator==(const HazelTexture& other) const = 0;

		virtual uint32_t GetID() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual void SetData(void* data, uint32_t size);
		static uint32_t GetBPP(HazelImageFormat format);
		static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height);
	};

	class HazelTexture2D : public HazelTexture
	{
	public:
		static Ref<HazelTexture2D> Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		static Ref<HazelTexture2D> Create(HazelImageFormat format, uint32_t width, uint32_t height, HazelTextureWrap wrap = HazelTextureWrap::Clamp);
		static Ref<HazelTexture2D> Create(const std::string& path, bool srgb = false, HazelTextureWrap wrap = HazelTextureWrap::Clamp);

		virtual Ref<HazelImage2D> GetImage() const = 0;

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual Buffer GetWriteableBuffer() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual bool Loaded() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual HazelTextureType GetType() const override { return HazelTextureType::Texture2D; }

	};

	class HazelTextureCube : public HazelTexture
	{
	public:
		static Ref<HazelTextureCube> Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		static Ref<HazelTextureCube> Create(const std::string& path);

		virtual const std::string& GetPath() const = 0;

		virtual HazelTextureType GetType() const override { return HazelTextureType::TextureCube; }

	};

}
