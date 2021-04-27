#include "RendererContext.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/OpenGL/OpenGLContext.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"

namespace Hazel {

	Ref<RendererContext> RendererContext::Create(GLFWwindow* windowHandle)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPIType:None is currently not supported!"); return Ref<RendererContext>();
		case RendererAPI::RendererAPIType::OpenGL:  return Ref<OpenGLContext>::Create(windowHandle);
		case RendererAPI::RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create(windowHandle);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<RendererContext>();
	}

}
