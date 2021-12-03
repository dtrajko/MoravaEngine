#include "RendererContextH2M.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "H2M/Platform/OpenGL/OpenGLContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "Platform/DX11/DX11Context.h"

namespace H2M
{

	RefH2M<RendererContextH2M> RendererContextH2M::Create(Window* window)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    HZ_CORE_ASSERT(false, "RendererAPIType:None is currently not supported!"); return RefH2M<RendererContextH2M>();
			case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLContextH2M>::Create(window);
			case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanContextH2M>::Create(window);
			case RendererAPITypeH2M::DX11:    return RefH2M<DX11Context>::Create(window);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<RendererContextH2M>();
	}

}
