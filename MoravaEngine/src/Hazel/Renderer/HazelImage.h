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
		static Ref<HazelImage2D> Create(HazelImageFormat format, uint32_t width, uint32_t height, Buffer buffer);
		static Ref<HazelImage2D> Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
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

	}

}
