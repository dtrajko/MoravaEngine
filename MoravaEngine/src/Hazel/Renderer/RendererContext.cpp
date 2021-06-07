#include "RendererContext.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/OpenGL/OpenGLContext.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Platform/DX11/DX11Context.h"

namespace Hazel {

	Ref<RendererContext> RendererContext::Create(GLFWwindow* windowHandle)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPIType:None is currently not supported!"); return Ref<RendererContext>();
		case RendererAPIType::OpenGL:  return Ref<OpenGLContext>::Create(windowHandle);
		case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create(windowHandle);
		case RendererAPIType::DX11:    return Ref<DX11Context>::Create(windowHandle);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<RendererContext>();
	}

}
