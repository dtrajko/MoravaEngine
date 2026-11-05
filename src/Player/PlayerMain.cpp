// MoravaPlayer: runs a game on MoravaCore alone, without the editor and without ImGui
// (docs/engine/Game_Engine_Runtime_Guide.html, Phase 1): MoravaPlayer --project <file.mproject> plays the project's start
// scene in the project's window; MoravaPlayer --scene <file.mscene> plays a scene (see PlayerLayer).

#include "PlayerApplication.h"

#include "Assets/AssetFiles.h"
#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/Process.h"

#include "H2M/Core/LogH2M.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Project/Project.h"

#include <filesystem>
#include <memory>


int main(int argc, char** argv)
{
	Log::Init();
	H2M::LogH2M::Init(); // H2M_ERROR / H2M_CORE_ERROR and failed H2M asserts log through these loggers

	// The game's pack: named, or Game.mpak here or beside the executable when nothing else is named
	const ApplicationCommandLine commandLine = ApplicationCommandLine::Parse(argc, argv);
	std::string packPath = commandLine.PackPath;
	if (packPath.empty() && commandLine.ProjectPath.empty() && commandLine.ScenePath.empty())
	{
		std::error_code error;
		for (const std::filesystem::path& candidate : { std::filesystem::path(AssetFiles::DefaultPackName), Process::GetExecutablePath().parent_path() / AssetFiles::DefaultPackName })
		{
			if (std::filesystem::is_regular_file(candidate, error))
			{
				packPath = candidate.u8string();
				break;
			}
		}
	}
	if (!packPath.empty())
	{
		std::string error;
		if (!AssetFiles::MountPack(packPath, error))
		{
			Log::GetLogger()->error("Pack '{0}' could not be opened: {1}", packPath, error);
			return 1;
		}
	}
	else if (!Application::IsWorkingDirectoryValid())
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
	if (AssetFiles::IsPackMounted() || !projectPath.empty())
	{
		Project project;
		std::string error;
		if (!(AssetFiles::IsPackMounted() ? Project::LoadFromPack(project, error) : Project::Load(projectPath, project, error)))
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
