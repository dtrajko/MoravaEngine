/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once


#include "H2M/Renderer/ImageH2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/RendererTypesH2M.h"

#include <GL/glew.h>


namespace H2M {

	class OpenGLImage2D_H2M : public Image2D_H2M
	{
	public:
		OpenGLImage2D_H2M(ImageSpecificationH2M specification);
		OpenGLImage2D_H2M(ImageSpecificationH2M specification, BufferH2M buffer);
		OpenGLImage2D_H2M(ImageSpecificationH2M specification, const void* data);

		OpenGLImage2D_H2M(ImageFormatH2M format, uint32_t width, uint32_t height, BufferH2M buffer);
		OpenGLImage2D_H2M(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data = nullptr);
		virtual ~OpenGLImage2D_H2M();

		virtual void Invalidate() override;
		void InvalidateOld(bool srgb, TextureWrapH2M wrap, bool isHDR); // Temporary method
		virtual void Release() override;

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual float GetAspectRatio() const override { return (float)m_Specification.Width / (float)m_Specification.Height; }

		virtual ImageSpecificationH2M& GetSpecification() override { return m_Specification; }
		virtual const ImageSpecificationH2M& GetSpecification() const override { return m_Specification; }

		void RT_Invalidate() {}

		virtual BufferH2M GetBuffer() const override { return m_ImageData; }
		virtual BufferH2M& GetBuffer() override { return m_ImageData; }

		virtual void CreatePerLayerImageViews() override {};

		virtual uint64_t GetHash() const override { return (uint64_t)m_RendererID; }

		RendererID_H2M& GetRendererID() { return m_RendererID; }
		RendererID_H2M GetRendererID() const { return m_RendererID; }

		RendererID_H2M& GetSamplerRendererID() { return m_SamplerRendererID; }
		RendererID_H2M GetSamplerRendererID() const { return m_SamplerRendererID; }

	private:
		ImageSpecificationH2M m_Specification;

		RendererID_H2M m_RendererID = 0;
		RendererID_H2M m_SamplerRendererID = 0;
		uint32_t m_Width, m_Height;
		ImageFormatH2M m_Format;

		BufferH2M m_ImageData;
	};

	namespace Utils {

		inline GLenum OpenGLImageFormat(ImageFormatH2M format)
		{
			switch (format)
			{
				case ImageFormatH2M::RGB:     return GL_RGB;
				case ImageFormatH2M::SRGB:    return GL_RGB;
				case ImageFormatH2M::RGBA:
				case ImageFormatH2M::RGBA16F:
				case ImageFormatH2M::RGBA32F: return GL_RGBA;
			}
			H2M_CORE_ASSERT(false, "Unknown image format");
			return 0;
		}

		inline GLenum OpenGLImageInternalFormat(ImageFormatH2M format)
		{
			switch (format)
			{
				case ImageFormatH2M::RGB:             return GL_RGB8;
				case ImageFormatH2M::SRGB:            return GL_SRGB8;
				case ImageFormatH2M::RGBA:            return GL_RGBA8;
				case ImageFormatH2M::RGBA16F:         return GL_RGBA16F;
				case ImageFormatH2M::RGBA32F:         return GL_RGBA32F;
				case ImageFormatH2M::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
				case ImageFormatH2M::DEPTH32F:        return GL_DEPTH_COMPONENT32F;
			}
			H2M_CORE_ASSERT(false, "Unknown image format");
			return 0;
		}

		inline GLenum OpenGLFormatDataType(ImageFormatH2M format)
		{
			switch (format)
			{
				case ImageFormatH2M::RGB:
				case ImageFormatH2M::SRGB:
				case ImageFormatH2M::RGBA:    return GL_UNSIGNED_BYTE;
				case ImageFormatH2M::RGBA16F:
				case ImageFormatH2M::RGBA32F: return GL_FLOAT;
			}
			H2M_CORE_ASSERT(false, "Unknown image format");
			return 0;
		}

		inline GLenum OpenGLSamplerWrap(TextureWrapH2M wrap)
		{
			switch (wrap)
			{
				case TextureWrapH2M::Clamp:   return GL_CLAMP_TO_EDGE;
				case TextureWrapH2M::Repeat:  return GL_REPEAT;
			}
			H2M_CORE_ASSERT(false, "Unknown wrap mode");
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
			H2M_CORE_ASSERT(false, "Unknown filter");
			return 0;
		}
		****/

	}

}
