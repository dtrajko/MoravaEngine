// MoravaPlayer: runs a game on MoravaCore alone, without the editor and without ImGui
// (docs/engine/Game_Engine_Runtime_Guide.html, Phase 1): MoravaPlayer --project <file.mproject> plays the project's start
// scene in the project's window; MoravaPlayer --scene <file.mscene> plays a scene (see PlayerLayer).

#include "PlayerApplication.h"
#include "PlayerProduct.h"

#include "Assets/AssetFiles.h"
#include "Core/Application.h"
#include "Core/FrameLimiter.h"
#include "Core/Log.h"
#include "Core/Process.h"

#include "Game/GameSettings.h"

#include "H2M/Core/LogH2M.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#include "H2M/Platform/Vulkan/VulkanFrameSyncH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Project/Project.h"

#include "Renderer/VulkanSupport.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>


int main(int argc, char** argv)
{
	Log::Init();
	H2M::LogH2M::Init(); // H2M_ERROR / H2M_CORE_ERROR and failed H2M asserts log through these loggers

	// The game's pack: named, or when nothing else is named Game.mpak beside the executable (in a macOS app: in its Resources),
	// or else in the current folder. A game beside its pack works in that folder, so a shortcut or another folder it is started
	// from doesn't matter
	const ApplicationCommandLine commandLine = ApplicationCommandLine::Parse(argc, argv);
	const bool startedAsGame = commandLine.PackPath.empty() && commandLine.ProjectPath.empty() && commandLine.ScenePath.empty();
	PlayerProduct::SetMessagesShown(startedAsGame && commandLine.ExitAfterSeconds <= 0.0f);
	std::string packPath = commandLine.PackPath;
	if (packPath.empty() && commandLine.ProjectPath.empty())
	{
		std::error_code error;
		const std::filesystem::path folder = Process::GetResourceFolder();
		if (std::filesystem::is_regular_file(folder / AssetFiles::DefaultPackName, error))
		{
			packPath = (folder / AssetFiles::DefaultPackName).u8string();
			std::filesystem::current_path(folder, error);
		}
		else if (commandLine.ScenePath.empty() && std::filesystem::is_regular_file(AssetFiles::DefaultPackName, error))
		{
			packPath = AssetFiles::DefaultPackName;
		}
	}
	if (!packPath.empty())
	{
		std::string error;
		if (!AssetFiles::MountPack(packPath, error))
		{
			PlayerProduct::ShowFatalMessage("The game's data file can't be read (" + error + "). Please install the game again.\n\n" + packPath);
			return 1;
		}
	}
	else if (startedAsGame)
	{
		Log::GetLogger()->info("To play something else than a built game: --project <file.mproject> or --scene <file.mscene>");
		PlayerProduct::ShowFatalMessage(std::string("The game's data file (") + AssetFiles::DefaultPackName + ") is missing. Please install the game again.");
		return 1;
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
	H2M::VulkanFrameSync::SetFramesInFlight((uint32_t)Application::Get()->GetCommandLine().FramesInFlight); // before the swapchain
	FrameLimiter::ForceOnBattery(commandLine.OnBattery);

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
			PlayerProduct::ShowFatalMessage("The game's project could not be opened (" + error + "). Please install the game again.");
			return 1;
		}
		Project::SetOpen(project);
		PlayerProduct::SetProject(project);
		if (project.Shipped)
		{
			PlayerProduct::StartLogFile(project, commandLine.Verbose);
			PlayerProduct::InstallCrashHandler(project);
			PlayerProduct::StopOnDamagedFiles();
		}
		// The project's window, as the player last chose it (11.4: the settings in the user's folder)
		const PlayerSettings player = GameSettings::ReadPlayerSettings(GameSettings::GetUserFile(project, GameSettings::SettingsFile));
		// A developer's pack older than its project (a scene, a model or the scripts changed since it was built): the game runs as
		// the pack has it, not as the editor shows it. Said in the log and in the window's title
		std::string title = project.GetWindowTitle();
		if (AssetFiles::IsPackMounted() && !project.Shipped)
		{
			const std::vector<std::string> changed = AssetFiles::FindChangedPackSources();
			if (!changed.empty())
			{
				constexpr size_t Named = 10;
				Log::GetLogger()->warn("The pack '{0}' is older than its project: {1} file(s) changed since it was built. Build it again "
					"(MoravaEngine --build-pack <file.mproject>)", AssetFiles::GetPackFile(), changed.size());
				for (size_t i = 0; i < changed.size() && i < Named; i++)
				{
					Log::GetLogger()->warn("  changed: {0}", changed[i]);
				}
				if (changed.size() > Named)
				{
					Log::GetLogger()->warn("  and {0} more", changed.size() - Named);
				}
				title += " (the pack is out of date: " + std::to_string(changed.size()) + " file(s) changed)";
			}
		}
		window = WindowSpecification(title, project.Window.Width, project.Window.Height);
		window.Fullscreen = player.Fullscreen.value_or(project.Window.Fullscreen);
		window.VSync = player.VSync.value_or(project.Window.VSync);
		// The frame rate limits (FrameLimiter): the player's, else the project's; --fps-limit for this run
		FrameLimiter::SetLimit(commandLine.FrameRateLimit >= 0 ? commandLine.FrameRateLimit : player.FrameRateLimit.value_or(project.Window.FrameRateLimit));
		FrameLimiter::SetBatteryLimit(player.BatteryFrameRateLimit.value_or(project.Window.BatteryFrameRateLimit));
		if (player.Resolution)
		{
			window.FullscreenWidth = player.Resolution->Width;
			window.FullscreenHeight = player.Resolution->Height;
			if (!window.Fullscreen)
			{
				// A window of that size (PlayerLayer fits it to the monitor and centers it)
				window.Width = player.Resolution->Width;
				window.Height = player.Resolution->Height;
			}
		}
	}
	if (!commandLine.CrashTest.empty() && commandLine.CrashTest != "frame")
	{
		PlayerProduct::Crash(commandLine.CrashTest);
	}

	// Before a window: a machine that can't run the renderer is told so
	std::string reason;
	if (!VulkanSupport::Check(reason))
	{
		PlayerProduct::ShowFatalMessage(reason + "\n\nInstalling the latest driver for the graphics card may help.");
		return 1;
	}

	Application::Get()->InitWindow(window);

	Application::Get()->Run();

	PlayerProduct::FlushLog();
	return Application::Get()->GetExitCode();
}
