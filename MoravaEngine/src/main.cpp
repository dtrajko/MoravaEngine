#include "main.h"

#include "H2M/Core/LogH2M.h"
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#endif

#include <cstdlib>
#include <filesystem>
#include <string>


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


// Command line options:
//   --scene <file>          open this scene at startup (SceneEnvMapVulkan: an .mscene file, e.g. Scenes/Old_Stove_Corner.mscene)
//   --exit-after <seconds>  close the application after this time in the main loop
//   --debug-view <number>   start with this debug view (SceneEnvMapVulkan: the entry of the Environment panel's Debug View
//                           list, 0 = Off, e.g. 8 = Probe Irradiance Only, 10 = Ambient Occlusion)
//   --no-ray-tracing        Vulkan without the ray tracing extensions, as on a GPU that has none (to check the fallbacks)
//   --select <name>         select the entity of this name in the scene opened with --scene (SceneEnvMapVulkan: a name of
//                           the Scene Hierarchy, e.g. Gladiator.fbx)
static ApplicationCommandLine ParseCommandLine(int argc, char** argv)
{
	ApplicationCommandLine commandLine;
	for (int i = 1; i < argc; i++)
	{
		const std::string option = argv[i];
		const bool hasValue = i + 1 < argc;
		if (option == "--scene" && hasValue)
		{
			commandLine.ScenePath = argv[++i];
		}
		else if (option == "--exit-after" && hasValue)
		{
			const char* value = argv[++i];
			char* end = nullptr;
			float seconds = std::strtof(value, &end);
			if (end == value || *end != '\0' || seconds <= 0.0f)
			{
				Log::GetLogger()->warn("Command line: --exit-after needs a number of seconds above 0, got '{0}' (ignored)", value);
			}
			else
			{
				commandLine.ExitAfterSeconds = seconds;
			}
		}
		else if (option == "--debug-view" && hasValue)
		{
			const char* value = argv[++i];
			char* end = nullptr;
			long view = std::strtol(value, &end, 10);
			if (end == value || *end != '\0' || view < 0)
			{
				Log::GetLogger()->warn("Command line: --debug-view needs a number from 0 on, got '{0}' (ignored)", value);
			}
			else
			{
				commandLine.DebugView = (int)view;
			}
		}
		else if (option == "--no-ray-tracing")
		{
			commandLine.NoRayTracing = true;
		}
		else if (option == "--select" && hasValue)
		{
			commandLine.SelectName = argv[++i];
		}
		else if (option == "--scene" || option == "--exit-after" || option == "--debug-view" || option == "--select")
		{
			Log::GetLogger()->warn("Command line: {0} needs a value (ignored)", option);
		}
		else
		{
			Log::GetLogger()->warn("Command line: unknown option '{0}' (ignored)", option);
		}
	}
	return commandLine;
}

// Shaders, models, textures and scenes are loaded relative to the working directory, which must be the
// MoravaEngine/MoravaEngine folder of the repository. Started anywhere else, the first shader comes back empty
// and the engine fails much later with an unrelated error, so say it here.
static bool IsWorkingDirectoryValid()
{
	std::error_code error;
	if (std::filesystem::is_directory("Resources/Shaders", error))
	{
		return true;
	}
	const std::string folder = std::filesystem::current_path(error).u8string();
	Log::GetLogger()->error("Resources/Shaders not found in the working directory '{0}'.", folder);
	Log::GetLogger()->error("Start MoravaEngine from the MoravaEngine/MoravaEngine folder of the repository.");
	return false;
}

int main(int argc, char** argv)
{
	Log::Init();
	H2M::LogH2M::Init(); // H2M_ERROR / H2M_CORE_ERROR and failed H2M asserts log through these loggers

	if (!IsWorkingDirectoryValid())
	{
		return 1;
	}

	SelectRendererAPI();

	Application::Get()->SetCommandLine(ParseCommandLine(argc, argv));
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
