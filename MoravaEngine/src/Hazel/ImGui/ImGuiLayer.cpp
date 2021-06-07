#include "ImGuiLayer.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/OpenGL/OpenGLImGuiLayer.h"
#include "Hazel/Platform/Vulkan/VulkanImGuiLayer.h"
#include "Platform/DX11/DX11ImGuiLayer.h"


namespace Hazel {


	ImGuiLayer* ImGuiLayer::Create()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:   return nullptr;
		case RendererAPIType::OpenGL: return new OpenGLImGuiLayer();
		case RendererAPIType::Vulkan: return new VulkanImGuiLayer();
		case RendererAPIType::DX11:   return new DX11ImGuiLayer();
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
