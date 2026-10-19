#include "main.h"

#include "EditorApplication.h"

#include "H2M/Core/LogH2M.h"
#if defined(GRAPHICS_API_VULKAN)
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

	Application::Get()->InitWindow(WindowSpecification("Morava Engine", 1280, 720));

	Application::Get()->Run();

	return 0;
}
