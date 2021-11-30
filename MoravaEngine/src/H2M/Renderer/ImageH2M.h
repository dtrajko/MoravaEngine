#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/Base.h"
#include "H2M/Core/Buffer.h"
#include "H2M/Core/Ref.h"


namespace H2M {

	enum class ImageFormatH2M
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

	enum class ImageUsageH2M
	{
		None = 0,
		Texture,
		Attachment,
		Storage,
	};

	enum class TextureWrapH2M
	{
		None = 0,
		Clamp,
		Repeat,
	};

	enum class TextureFilterH2M
	{
		None = 0,
		Linear,
		Nearest,
	};

	enum class TextureTypeH2M
	{
		None = 0,
		Texture2D,
		TextureCube,
	};

	struct TexturePropertiesH2M
	{
		TextureWrapH2M SamplerWrap = TextureWrapH2M::Repeat;
		TextureFilterH2M SamplerFilter = TextureFilterH2M::Linear;
		bool GenerateMips = true;
		bool SRGB = false;
		bool Storage = false;

		std::string DebugName;
	};

	struct ImageSpecification
	{
		ImageFormatH2M Format = ImageFormatH2M::RGBA;
		ImageUsageH2M Usage = ImageUsageH2M::Texture;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Mips = 1;
		uint32_t Layers = 1;
		bool Deinterleaved = false;

		std::string DebugName;
	};

	class ImageH2M : public H2M::RefCounted
	{
	public:
		virtual ~ImageH2M() {}

		virtual void Invalidate() = 0;
		virtual void Release() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual float GetAspectRatio() const = 0;

		virtual ImageSpecification& GetSpecification() = 0;
		virtual const ImageSpecification& GetSpecification() const = 0;

		virtual H2M::Buffer GetBuffer() const = 0;
		virtual H2M::Buffer& GetBuffer() = 0;

		virtual void CreatePerLayerImageViews() = 0;

		virtual uint64_t GetHash() const = 0;

		// TODO: usage (eg. shader read)
	};

	class Image2DH2M : public ImageH2M
	{
	public:
		static H2M::Ref<Image2DH2M> Create(ImageSpecification specification, H2M::Buffer buffer);
		static H2M::Ref<Image2DH2M> Create(ImageSpecification specification, const void* data = nullptr);
	};

	namespace Utils {

		inline uint32_t GetImageFormatBPP(ImageFormatH2M format)
		{
			switch (format)
			{
			case ImageFormatH2M::RED32F:  return 4;
			case ImageFormatH2M::RGB:
			case ImageFormatH2M::SRGB:    return 3;
			case ImageFormatH2M::RGBA:    return 4;
			case ImageFormatH2M::RGBA16F: return 2 * 4;
			case ImageFormatH2M::RGBA32F: return 4 * 4;
			}
			HZ_CORE_ASSERT(false);
			return 0;
		}

		inline uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::floor(std::log2(glm::min(width, height))) + 1;
		}

		inline uint32_t GetImageMemorySize(ImageFormatH2M format, uint32_t width, uint32_t height)
		{
			return width * height * GetImageFormatBPP(format);
		}

		inline bool IsDepthFormat(ImageFormatH2M format)
		{
			if (format == ImageFormatH2M::DEPTH24STENCIL8 || format == ImageFormatH2M::DEPTH32F)
				return true;

			return false;
		}

	}

}
