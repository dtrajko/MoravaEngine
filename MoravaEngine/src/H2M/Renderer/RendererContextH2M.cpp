/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "RendererContextH2M.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#if defined(GRAPHICS_API_OPENGL)
#include "H2M/Platform/OpenGL/OpenGLContextH2M.h"
#endif
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#endif
#if defined(GRAPHICS_API_DIRECTX11)
#include "Platform/DX11/DX11Context.h"
#endif

namespace H2M
{

	RefH2M<RendererContextH2M> RendererContextH2M::Create(Window* window)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    H2M_CORE_ASSERT(false, "RendererAPITypeH2M:None is currently not supported!"); return RefH2M<RendererContextH2M>();
#if defined(GRAPHICS_API_OPENGL)
			case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLContextH2M>::Create(window);
#endif
#if defined(GRAPHICS_API_VULKAN)
			case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanContextH2M>::Create(window);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
			case RendererAPITypeH2M::DX11:    return RefH2M<DX11Context>::Create(window);
#endif
		}
		Log::GetLogger()->error("Unknown RendererAPI_H2M");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI_H2M");
		return RefH2M<RendererContextH2M>();
	}

}
