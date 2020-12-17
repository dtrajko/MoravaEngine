#pragma once

#include "../Core/Base.h"
#include "../Core/Ref.h"
#include "../Core/Buffer.h"


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

	class HazelTexture : public RefCounted
	{
	public:
		virtual ~HazelTexture() {}

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual HazelTextureFormat GetFormat() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;

		virtual uint32_t GetID() const = 0;

		virtual void SetData(void* data, uint32_t size);

		static uint32_t GetBPP(HazelTextureFormat format);
		static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height);

		virtual bool operator==(const HazelTexture& other) const = 0;
	};

	class HazelTexture2D : public HazelTexture
	{
	public:
		static HazelTexture2D* Create(HazelTextureFormat format, uint32_t width, uint32_t height, HazelTextureWrap wrap = HazelTextureWrap::Clamp);
		static HazelTexture2D* Create(const std::string& path, bool srgb = false);

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual Buffer GetWriteableBuffer() = 0;

		virtual bool Loaded() const = 0;

		virtual const std::string& GetPath() const = 0;
	};

	class HazelTextureCube : public HazelTexture
	{
	public:
		static HazelTextureCube* Create(HazelTextureFormat format, uint32_t width, uint32_t height);
		static HazelTextureCube* Create(const std::string& path);

		virtual const std::string& GetPath() const = 0;
	};

}
