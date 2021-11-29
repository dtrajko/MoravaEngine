#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Buffer.h"
#include "Hazel/Core/Ref.h"


namespace HazelLegacy {

	enum class ImageFormatHazelLegacy
	{
		None = 0,
		RED32F,
		RGB,
		RGBA,
		RGBA16F,
		RGBA32F,
		RG16F,
		RG32F,

		SRGB,

		DEPTH32F,
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8,
	};

	enum class ImageUsageHazelLegacy
	{
		None = 0,
		Texture,
		Attachment,
		Storage,
	};

	enum class TextureWrapHazelLegacy
	{
		None = 0,
		Clamp,
		Repeat,
	};

	enum class TextureFilterHazelLegacy
	{
		None = 0,
		Linear,
		Nearest,
	};

	enum class TextureTypeHazelLegacy
	{
		None = 0,
		Texture2D,
		TextureCube,
	};

	struct TexturePropertiesHazelLegacy
	{
		TextureWrapHazelLegacy SamplerWrap = TextureWrapHazelLegacy::Repeat;
		TextureFilterHazelLegacy SamplerFilter = TextureFilterHazelLegacy::Linear;
		bool GenerateMips = true;
		bool SRGB = false;
		bool Storage = false;

		std::string DebugName;
	};

	struct ImageSpecification
	{
		ImageFormatHazelLegacy Format = ImageFormatHazelLegacy::RGBA;
		ImageUsageHazelLegacy Usage = ImageUsageHazelLegacy::Texture;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Mips = 1;
		uint32_t Layers = 1;
		bool Deinterleaved = false;

		std::string DebugName;
	};

	class ImageHazelLegacy : public Hazel::RefCounted
	{
	public:
		virtual ~ImageHazelLegacy() {}

		virtual void Invalidate() = 0;
		virtual void Release() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual float GetAspectRatio() const = 0;

		virtual ImageSpecification& GetSpecification() = 0;
		virtual const ImageSpecification& GetSpecification() const = 0;

		virtual Hazel::Buffer GetBuffer() const = 0;
		virtual Hazel::Buffer& GetBuffer() = 0;

		virtual void CreatePerLayerImageViews() = 0;

		virtual uint64_t GetHash() const = 0;

		// TODO: usage (eg. shader read)
	};

	class Image2DHazelLegacy : public ImageHazelLegacy
	{
	public:
		static Hazel::Ref<Image2DHazelLegacy> Create(ImageSpecification specification, Hazel::Buffer buffer);
		static Hazel::Ref<Image2DHazelLegacy> Create(ImageSpecification specification, const void* data = nullptr);
	};

	namespace Utils {

		inline uint32_t GetImageFormatBPP(ImageFormatHazelLegacy format)
		{
			switch (format)
			{
			case ImageFormatHazelLegacy::RED32F:  return 4;
			case ImageFormatHazelLegacy::RGB:
			case ImageFormatHazelLegacy::SRGB:    return 3;
			case ImageFormatHazelLegacy::RGBA:    return 4;
			case ImageFormatHazelLegacy::RGBA16F: return 2 * 4;
			case ImageFormatHazelLegacy::RGBA32F: return 4 * 4;
			}
			HZ_CORE_ASSERT(false);
			return 0;
		}

		inline uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::floor(std::log2(glm::min(width, height))) + 1;
		}

		inline uint32_t GetImageMemorySize(ImageFormatHazelLegacy format, uint32_t width, uint32_t height)
		{
			return width * height * GetImageFormatBPP(format);
		}

		inline bool IsDepthFormat(ImageFormatHazelLegacy format)
		{
			if (format == ImageFormatHazelLegacy::DEPTH24STENCIL8 || format == ImageFormatHazelLegacy::DEPTH32F)
				return true;

			return false;
		}

	}

}
