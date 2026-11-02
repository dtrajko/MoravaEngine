/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "ImageH2M.h"

#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanImageH2M.h"
#endif
#if defined(GRAPHICS_API_OPENGL)
#include "H2M/Platform/OpenGL/OpenGLImageH2M.h"
#endif
#include "H2M/Renderer/RendererAPI_H2M.h"
#if defined(GRAPHICS_API_DIRECTX11)
#include "Platform/DX11/DX11Image.h"
#endif


namespace H2M
{

	RefH2M<Image2D_H2M> Image2D_H2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, BufferH2M buffer)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:   return RefH2M<Image2D_H2M>();
#if defined(GRAPHICS_API_OPENGL)
		case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLImage2D_H2M>::Create(format, width, height, buffer);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanImage2D_H2M>::Create(format, width, height);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case RendererAPITypeH2M::DX11:   return RefH2M<DX11Image2D>::Create(format, width, height);
#endif
		default: break;
		}
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Image2D_H2M>();
	}

	RefH2M<Image2D_H2M> Image2D_H2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:   return RefH2M<Image2D_H2M>();
#if defined(GRAPHICS_API_OPENGL)
		case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLImage2D_H2M>::Create(format, width, height, data);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanImage2D_H2M>::Create(format, width, height);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case RendererAPITypeH2M::DX11:   return RefH2M<DX11Image2D>::Create(format, width, height);
#endif
		default: break;
		}
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Image2D_H2M>();
	}

	RefH2M<Image2D_H2M> Image2D_H2M::Create(ImageSpecificationH2M specification, const void* data)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:   return RefH2M<Image2D_H2M>();
#if defined(GRAPHICS_API_OPENGL)
		case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLImage2D_H2M>::Create(specification);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanImage2D_H2M>::Create(specification);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case RendererAPITypeH2M::DX11:   return RefH2M<DX11Image2D>::Create(specification);
#endif
		default: break;
		}
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Image2D_H2M>();
	}

}
