/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/BufferH2M.h"
#include "H2M/Core/RefH2M.h"


namespace H2M
{

	enum class ImageFormatH2M
	{
		None = 0,
		RGB,
		RGBA,
		RGBA8,
		RGBA16F,
		RGBA32F,
		RG32F,

		RED_INTEGER,

		SRGB,

		DEPTH32F,
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8
	};

	enum class ImageUsageH2M
	{
		None = 0,
		Texture,
		Attachment,
		Storage
	};

	enum class TextureWrapH2M
	{
		None = 0,
		Clamp,
		Repeat
	};

	enum class TextureFilterH2M
	{
		None = 0,
		Linear,
		Nearest
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

	struct ImageSpecificationH2M
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

	class ImageH2M : public RefCountedH2M
	{
	public:
		virtual ~ImageH2M() {}

		virtual void Invalidate() = 0;
		virtual void Release() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual ImageFormatH2M GetFormat() const = 0;
		virtual float GetAspectRatio() const = 0;

		virtual BufferH2M GetBuffer() const = 0;
		virtual BufferH2M& GetBuffer() = 0;

		virtual uint64_t GetHash() const = 0;

		// TODO: usage (eg. shader read)
	};

	class Image2D_H2M : public ImageH2M
	{
	public:
		static RefH2M<Image2D_H2M> Create(ImageFormatH2M format, uint32_t width, uint32_t height, BufferH2M buffer);
		static RefH2M<Image2D_H2M> Create(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data = nullptr);
	};

	namespace Utils {

		inline uint32_t GetImageFormatBPP(ImageFormatH2M format) // BPP - bytes per pixel
		{
			switch (format)
			{
				case ImageFormatH2M::RGB:
				case ImageFormatH2M::SRGB:    return 3;
				case ImageFormatH2M::RGBA:    return 4;
				case ImageFormatH2M::RGBA8:   return 4;
				case ImageFormatH2M::RGBA16F: return 2 * 4;
				case ImageFormatH2M::RGBA32F: return 4 * 4;
			}
			H2M_CORE_ASSERT(false);
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
			{
				return true;
			}

			return false;
		}

	}

}
