// MoravaPlayer: runs a game on MoravaCore alone, without the editor and without ImGui
// (docs/engine/Game_Engine_Runtime_Guide.html, Phase 1): MoravaPlayer --project <file.mproject> plays the project's start
// scene in the project's window; MoravaPlayer --scene <file.mscene> plays a scene (see PlayerLayer).

#include "PlayerApplication.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include "H2M/Core/LogH2M.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Project/Project.h"

#include <memory>


int main(int argc, char** argv)
{
	Log::Init();
	H2M::LogH2M::Init(); // H2M_ERROR / H2M_CORE_ERROR and failed H2M asserts log through these loggers

	if (!Application::IsWorkingDirectoryValid())
	{
		return 1;
	}

	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::Vulkan);

	Application::Get()->SetClient(std::make_unique<PlayerApplication>());
	Application::Get()->SetCommandLine(ApplicationCommandLine::Parse(argc, argv)); // the options: see ApplicationCommandLine::Parse
	if (Application::Get()->GetCommandLine().NoRayTracing)
	{
		H2M::VulkanPhysicalDeviceH2M::DisableRayTracing(); // before the window creates the Vulkan device
	}

	// The project's window; without a project (a scene alone, as in tests) a window that presents without waiting for the
	// display, as the editor's
	WindowSpecification window("Morava Player", 1280, 720);
	window.VSync = false;
	const std::string& projectPath = Application::Get()->GetCommandLine().ProjectPath;
	if (!projectPath.empty())
	{
		Project project;
		std::string error;
		if (!Project::Load(projectPath, project, error))
		{
			Log::GetLogger()->error("Project '{0}' could not be opened: {1}", projectPath, error);
			return 1;
		}
		Project::SetOpen(project);
		window = WindowSpecification(project.GetWindowTitle(), project.Window.Width, project.Window.Height);
		window.Fullscreen = project.Window.Fullscreen;
		window.VSync = project.Window.VSync;
	}
	Application::Get()->InitWindow(window);

	Application::Get()->Run();

	return Application::Get()->GetExitCode();
}
