#include "VertexBufferH2M.h"

#if defined(GRAPHICS_API_OPENGL)
#include "H2M/Platform/OpenGL/OpenGLVertexBufferH2M.h"
#endif
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanVertexBufferH2M.h"
#endif
#include "H2M/Renderer/RendererAPI_H2M.h"

#if defined(GRAPHICS_API_DIRECTX11)
#include "Platform/DX11/DX11VertexBuffer.h"
#endif


namespace H2M
{

	RefH2M<VertexBufferH2M> VertexBufferH2M::Create(void* data, uint32_t size, VertexBufferUsageH2M usage)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:    return RefH2M<VertexBufferH2M>();
#if defined(GRAPHICS_API_OPENGL)
		case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLVertexBufferH2M>::Create(data, size, usage);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanVertexBufferH2M>::Create(data, size, usage);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case RendererAPITypeH2M::DX11:    return RefH2M<DX11VertexBuffer>::Create(data, size, usage);
#endif
		default: break;
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<VertexBufferH2M>();
	}

	RefH2M<VertexBufferH2M> VertexBufferH2M::Create(uint32_t size, VertexBufferUsageH2M usage)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:    return RefH2M<VertexBufferH2M>();
#if defined(GRAPHICS_API_OPENGL)
		case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLVertexBufferH2M>::Create(size, usage);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanVertexBufferH2M>::Create(size, usage);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case RendererAPITypeH2M::DX11:    return RefH2M<DX11VertexBuffer>::Create(size, usage);
#endif
		default: break;
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<VertexBufferH2M>();
	}

	RefH2M<VertexBufferH2M> VertexBufferH2M::Create(void* data, uint32_t stride, uint32_t count, VertexBufferUsageH2M usage)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:    return RefH2M<VertexBufferH2M>();
#if defined(GRAPHICS_API_OPENGL)
		case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLVertexBufferH2M>::Create(data, stride * count, usage);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanVertexBufferH2M>::Create(data, stride * count, usage);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case RendererAPITypeH2M::DX11:    return RefH2M<DX11VertexBuffer>::Create(data, stride, count);
#endif
		default: break;
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<VertexBufferH2M>();
	}

}
