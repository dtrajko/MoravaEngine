/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "RendererContextH2M.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "H2M/Platform/OpenGL/OpenGLContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "Platform/DX11/DX11Context.h"

namespace H2M
{

	RendererContextH2M* RendererContextH2M::Create(Window* window)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    H2M_CORE_ASSERT(false, "RendererAPITypeH2M:None is currently not supported!"); return nullptr;
			case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLContextH2M>::Create(window).Raw();
			case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanContextH2M>::Create(window).Raw();
			case RendererAPITypeH2M::DX11:    return RefH2M<DX11Context>::Create(window).Raw();
		}
		Log::GetLogger()->error("Unknown RendererAPI_H2M");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI_H2M");
		return nullptr;
	}

}
