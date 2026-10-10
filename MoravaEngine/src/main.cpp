#include "main.h"

#include "H2M/Core/LogH2M.h"

#include <cstdlib>
#include <string>


void SelectRendererAPI()
{
#if defined(SCENE_HAZEL_VULKAN) ||\
	defined(SCENE_ENV_MAP_VULKAN)
	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::Vulkan);
#elif defined(SCENE_DX11) 
	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::DX11);
#else:
	H2M::RendererAPI_H2M::SetAPI(H2M::RendererAPITypeH2M::OpenGL);
#endif;
}


// Command line options:
//   --scene <file>          open this scene at startup (SceneEnvMapVulkan: an .mscene file, e.g. Scenes/Old_Stove_Corner.mscene)
//   --exit-after <seconds>  close the application after this time in the main loop
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
		else if (option == "--scene" || option == "--exit-after")
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

int main(int argc, char** argv)
{
	Log::Init();
	H2M::LogH2M::Init(); // H2M_ERROR / H2M_CORE_ERROR and failed H2M asserts log through these loggers

	SelectRendererAPI();

	Application::Get()->SetCommandLine(ParseCommandLine(argc, argv));

	Application::Get()->InitWindow(WindowSpecification("Morava Engine", 1280, 720));

	Application::Get()->Run();

	return 0;
}
