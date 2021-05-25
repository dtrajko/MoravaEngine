#include "ImGuiLayer.h"

#include "Hazel/Platform/OpenGL/OpenGLImGuiLayer.h"
#include "Hazel/Platform/Vulkan/VulkanImGuiLayer.h"
#include "Hazel/Renderer/RendererAPI.h"


namespace Hazel {


	ImGuiLayer* ImGuiLayer::Create()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return new OpenGLImGuiLayer();
		case RendererAPIType::Vulkan: return new VulkanImGuiLayer();
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
