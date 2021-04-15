#include "GraphicsContext.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Platform/OpenGL/OpenGLContext.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"


namespace Hazel {

	Ref<RendererContext> GraphicsContext::Create(GLFWwindow* window)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPIType:None is currently not supported!"); return Ref<RendererContext>();
		case RendererAPIType::OpenGL:  return Ref<OpenGLContext>::Create(window);
		case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create(window);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return Ref<GraphicsContext>();
	}

	void GraphicsContext::Init()
	{
	}

}
