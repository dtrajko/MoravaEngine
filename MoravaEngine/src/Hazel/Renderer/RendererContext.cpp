#include "RendererContext.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/OpenGL/OpenGLContext.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Platform/DX11/DX11Context.h"

namespace Hazel {

	Ref<RendererContext> RendererContext::Create(Window* window)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPIType:None is currently not supported!"); return Ref<RendererContext>();
			case RendererAPIType::OpenGL:  return Ref<OpenGLContext>::Create(window);
			case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create(window);
			case RendererAPIType::DX11:    return Ref<DX11Context>::Create(window);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<RendererContext>();
	}

}
