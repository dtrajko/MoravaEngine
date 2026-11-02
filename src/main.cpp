#include "main.h"

#include "EditorApplication.h"

#include "H2M/Core/LogH2M.h"
#if defined(GRAPHICS_API_VULKAN)
#include "Editor/Editor.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#endif

#include <memory>


void SelectRendererAPI()
{
#if defined(SCENE_HAZEL_VULKAN) ||\
	defined(SCENE_ENV_MAP_VULKAN)
	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::Vulkan);
#elif defined(SCENE_DX11) 
	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::DX11);
#else
	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::OpenGL);
#endif
}


int main(int argc, char** argv)
{
	Log::Init();
#if defined(GRAPHICS_API_VULKAN)
	Editor::InstallConsole(); // the editor (src/Editor) is in Vulkan builds only
#endif
	H2M::LogH2M::Init(); // H2M_ERROR / H2M_CORE_ERROR and failed H2M asserts log through these loggers

	if (!Application::IsWorkingDirectoryValid())
	{
		return 1;
	}

	SelectRendererAPI();

	Application::Get()->SetClient(std::make_unique<EditorApplication>());
	Application::Get()->SetCommandLine(ApplicationCommandLine::Parse(argc, argv)); // the options: see ApplicationCommandLine::Parse
#if defined(GRAPHICS_API_VULKAN)
	if (Application::Get()->GetCommandLine().NoRayTracing)
	{
		H2M::VulkanPhysicalDeviceH2M::DisableRayTracing(); // before the window creates the Vulkan device
	}
#endif

	WindowSpecification window("Morava Engine", 1280, 720);
	// Vulkan presents without waiting for the display (mailbox or immediate): the editor shows how fast a frame really is
	window.VSync = H2M::RendererAPI_H2M::Current() != H2M::RendererAPITypeH2M::Vulkan;
	Application::Get()->InitWindow(window);

	Application::Get()->Run();

	return 0;
}
