#define _CRT_SECURE_NO_WARNINGS

#pragma once


#include "Hazel/Renderer/HazelImage.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RendererTypes.h"

#include <GL/glew.h>


namespace Hazel {

	class OpenGLImage2D : public HazelImage2D
	{
	public:
		OpenGLImage2D(HazelImageFormat format, uint32_t width, uint32_t height, Buffer buffer);
		OpenGLImage2D(HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		virtual ~OpenGLImage2D();

		virtual void Invalidate() override;
		virtual void Release() override;

		virtual HazelImageFormat GetFormat() const override { return m_Format; }
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual float GetAspectRatio() const override { return (float)m_Specification.Width / (float)m_Specification.Height; }

		virtual Buffer GetBuffer() const override { return m_ImageData; }
		virtual Buffer& GetBuffer() override { return m_ImageData; }

		virtual uint64_t GetHash() const override { return (uint64_t)m_RendererID; }

		RendererID& GetRendererID() { return m_RendererID; }
		RendererID GetRendererID() const { return m_RendererID; }

		RendererID& GetSamplerRendererID() { return m_SamplerRendererID; }
		RendererID GetSamplerRendererID() const { return m_SamplerRendererID; }

	private:
		ImageSpecification m_Specification;

		RendererID m_RendererID = 0;
		RendererID m_SamplerRendererID = 0;
		uint32_t m_Width, m_Height;
		HazelImageFormat m_Format;

		Buffer m_ImageData;
	};

	namespace Utils {

		inline GLenum OpenGLImageFormat(HazelImageFormat format)
		{
			switch (format)
			{
				case HazelImageFormat::RGB:     return GL_RGB;
				case HazelImageFormat::SRGB:    return GL_RGB;
				case HazelImageFormat::RGBA:
				case HazelImageFormat::RGBA16F:
				case HazelImageFormat::RGBA32F: return GL_RGBA;
			}
			HZ_CORE_ASSERT(false, "Unknown image format");
			return 0;
		}

		inline GLenum OpenGLImageInternalFormat(HazelImageFormat format)
		{
			switch (format)
			{
				case HazelImageFormat::RGB:             return GL_RGB8;
				case HazelImageFormat::SRGB:            return GL_SRGB8;
				case HazelImageFormat::RGBA:            return GL_RGBA8;
				case HazelImageFormat::RGBA16F:         return GL_RGBA16F;
				case HazelImageFormat::RGBA32F:         return GL_RGBA32F;
				case HazelImageFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
				case HazelImageFormat::DEPTH32F:        return GL_DEPTH_COMPONENT32F;
			}
			HZ_CORE_ASSERT(false, "Unknown image format");
			return 0;
		}

		inline GLenum OpenGLFormatDataType(HazelImageFormat format)
		{
			switch (format)
			{
				case HazelImageFormat::RGB:
				case HazelImageFormat::SRGB:
				case HazelImageFormat::RGBA:    return GL_UNSIGNED_BYTE;
				case HazelImageFormat::RGBA16F:
				case HazelImageFormat::RGBA32F: return GL_FLOAT;
			}
			HZ_CORE_ASSERT(false, "Unknown image format");
			return 0;
		}

		inline GLenum OpenGLSamplerWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
				case TextureWrap::Clamp:   return GL_CLAMP_TO_EDGE;
				case TextureWrap::Repeat:  return GL_REPEAT;
			}
			HZ_CORE_ASSERT(false, "Unknown wrap mode");
			return 0;
		}

		/****
		// Note: should always be called with mipmap = false for magnification filtering
		inline GLenum OpenGLSamplerFilter(TextureFilter filter, bool mipmap)
		{
			switch (filter)
			{
				case TextureFilter::Linear:   return mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
				case TextureFilter::Nearest:  return mipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
			}
			HZ_CORE_ASSERT(false, "Unknown filter");
			return 0;
		}
		****/

	}

}
