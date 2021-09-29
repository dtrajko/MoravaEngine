#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Buffer.h"
#include "Hazel/Core/Ref.h"


namespace Hazel {

	enum class HazelImageFormat
	{
		None = 0,
		RGB,
		RGBA,
		RGBA16F,
		RGBA32F,
		RG32F,

		SRGB,

		DEPTH32F,
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8
	};

	enum class ImageUsage
	{
		None = 0,
		Texture,
		Attachment,
		Storage
	};

	struct ImageSpecification
	{
		HazelImageFormat Format = HazelImageFormat::RGBA;
		ImageUsage Usage = ImageUsage::Texture;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Mips = 1;
		uint32_t Layers = 1;
		bool Deinterleaved = false;

		std::string DebugName;
	};

	class HazelImage : public RefCounted
	{
	public:
		virtual ~HazelImage() {}

		virtual void Invalidate() = 0;
		virtual void Release() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual float GetAspectRatio() const = 0;

		virtual HazelImageFormat GetFormat() const = 0;

		virtual Buffer GetBuffer() const = 0;
		virtual Buffer& GetBuffer() = 0;

		virtual uint64_t GetHash() const = 0;

		// TODO: usage (eg. shader read)
	};

	class HazelImage2D : public HazelImage
	{
	public:
		static Ref<HazelImage2D> Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		static Ref<HazelImage2D> Create(HazelImageFormat format, uint32_t width, uint32_t height, Buffer buffer);

	};

	namespace Utils {

		inline uint32_t GetImageFormatBPP(HazelImageFormat format)
		{
			switch (format)
			{
			case HazelImageFormat::RGB:
			case HazelImageFormat::SRGB:    return 3;
			case HazelImageFormat::RGBA:    return 4;
			case HazelImageFormat::RGBA16F: return 2 * 4;
			case HazelImageFormat::RGBA32F: return 4 * 4;
			}
			HZ_CORE_ASSERT(false);
			return 0;
		}

		inline uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::floor(std::log2(glm::min(width, height))) + 1;
		}

		inline uint32_t GetImageMemorySize(HazelImageFormat format, uint32_t width, uint32_t height)
		{
			return width * height * GetImageFormatBPP(format);
		}

		inline bool IsDepthFormat(HazelImageFormat format)
		{
			if (format == HazelImageFormat::DEPTH24STENCIL8 || format == HazelImageFormat::DEPTH32F)
				return true;

			return false;
		}

	}

}
