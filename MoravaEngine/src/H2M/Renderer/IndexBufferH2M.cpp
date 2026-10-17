/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "IndexBufferH2M.h"

#if defined(GRAPHICS_API_OPENGL)
#include "H2M/Platform/OpenGL/OpenGLIndexBufferH2M.h"
#endif
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanIndexBufferH2M.h"
#endif
#include "H2M/Renderer/ModelH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#if defined(GRAPHICS_API_DIRECTX11)
#include "Platform/DX11/DX11IndexBuffer.h"
#endif


namespace H2M
{

	RefH2M<IndexBufferH2M> IndexBufferH2M::Create(uint32_t size)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    return RefH2M<IndexBufferH2M>();
#if defined(GRAPHICS_API_OPENGL)
			case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLIndexBufferH2M>::Create(size);
#endif
#if defined(GRAPHICS_API_VULKAN)
			case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanIndexBufferH2M>::Create(size);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
			case RendererAPITypeH2M::DX11:    return RefH2M<DX11IndexBuffer>::Create(size);
#endif
			default: break;
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<IndexBufferH2M>();
	}

	RefH2M<IndexBufferH2M> IndexBufferH2M::Create(void* data, uint32_t size)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    return RefH2M<IndexBufferH2M>();
#if defined(GRAPHICS_API_OPENGL)
			case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLIndexBufferH2M>::Create(data, size);
#endif
#if defined(GRAPHICS_API_VULKAN)
			case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanIndexBufferH2M>::Create(data, size);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
			case RendererAPITypeH2M::DX11:    return RefH2M<DX11IndexBuffer>::Create(data, size);
#endif
			default: break;
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<IndexBufferH2M>();
	}

}
