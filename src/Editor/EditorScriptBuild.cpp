// Building the project's C# scripts from the editor (docs/engine/Game_Engine_Runtime_Guide.html, 4.7): dotnet build of the
// project's script project (Project Settings: Script Project), on its own thread, when asked (Build Scripts, Ctrl+B) and
// when a script is saved (Build on Save: the scripts' folder is checked twice a second). The compiler's errors and warnings
// go to the log, as "Coin.cs(14,9): error CS0103: ...", which the Console panel opens at the line. A build that succeeds
// writes the assembly, which hot reload (EditorScriptReload.cpp) loads.


#include "Project/Project.h"
#include "Scripting/ScriptHost.h"

#include "Core/Log.h"
#include "Core/Process.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <set>
#include <string>
#include <system_error>
#include <thread>
#include <vector>


#include "EditorState.h"


namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

bool s_BuildScriptsOnSave = true;

static constexpr auto s_CheckInterval = std::chrono::milliseconds(500);
static constexpr auto s_SettleTime = std::chrono::milliseconds(300); // an editor saving several files: one build

static std::thread s_Worker;
static std::atomic<bool> s_Building { false };
static std::atomic<bool> s_WorkerDone { false };
static bool s_BuildRequested = false;
static std::mutex s_ResultMutex;
static ScriptBuildResult s_LastResult;     // the last finished build (under s_ResultMutex while the worker writes it)
static Clock::time_point s_StartedAt;

// Build on Save: the newest script's time when it was last looked at, for the project whose scripts they are
static std::string s_WatchedProject;
static fs::file_time_type s_NewestScript;
static Clock::time_point s_NextCheck;
static Clock::time_point s_ChangedAt;
static bool s_ChangeSeen = false;

// The .NET SDK's dotnet: on the PATH, in DOTNET_ROOT, else the one the engine was built with
static std::string FindDotNet()
{
	std::string dotnet = Process::FindOnPath("dotnet");
	if (!dotnet.empty())
	{
		return dotnet;
	}
	if (const char* root = std::getenv("DOTNET_ROOT"))
	{
		std::error_code error;
#if defined(_WIN32)
		const fs::path candidate = fs::u8path(root) / "dotnet.exe";
#else
		const fs::path candidate = fs::u8path(root) / "dotnet";
#endif
		if (fs::is_regular_file(candidate, error))
		{
			return candidate.u8string();
		}
	}
#if defined(MORAVA_DOTNET_SDK_EXE)
	return MORAVA_DOTNET_SDK_EXE;
#else
	return std::string();
#endif
}

std::string GetScriptProjectPath()
{
	const Project* project = Project::GetOpen();
	return project ? project->GetScriptProjectPath() : std::string();
}

bool IsScriptBuildRunning()
{
	return s_Building;
}

ScriptBuildResult GetLastScriptBuild()
{
	std::lock_guard<std::mutex> lock(s_ResultMutex);
	return s_LastResult;
}

void RequestScriptBuild()
{
	s_BuildRequested = true;
}

// A compiler line: "D:\Game\Scripts\Coin.cs(14,9): error CS0103: The name 'x' does not exist [D:\Game\Scripts\Game.csproj]".
// Written with the file relative to the project folder, without the project at the end
static bool ReadCompilerLine(const std::string& line, const fs::path& projectFolder, std::string& text, bool& isError)
{
	size_t marker = line.find("): error ");
	isError = marker != std::string::npos;
	if (!isError)
	{
		marker = line.find("): warning ");
	}
	if (marker == std::string::npos)
	{
		return false;
	}
	const size_t open = line.rfind('(', marker);
	if (open == std::string::npos)
	{
		return false;
	}
	size_t start = line.find_first_not_of(' ');
	fs::path file = fs::u8path(line.substr(start, open - start));
	std::error_code error;
	const fs::path relative = file.lexically_relative(projectFolder);
	if (!relative.empty() && *relative.begin() != "..")
	{
		file = relative;
	}
	std::string rest = line.substr(open);
	const size_t projectMark = rest.rfind(" [");
	if (projectMark != std::string::npos && rest.back() == ']')
	{
		rest.erase(projectMark);
	}
	text = file.generic_u8string() + rest;
	return true;
}

// The SDK's summary ("Build succeeded.", "    0 Warning(s)", "Time Elapsed 00:00:01.27"): the editor says it shorter
static bool IsBuildSummaryLine(const std::string& line)
{
	const size_t start = line.find_first_not_of(' ');
	const std::string text = start == std::string::npos ? std::string() : line.substr(start);
	if (text == "Build succeeded." || text == "Build FAILED." || text.rfind("Time Elapsed", 0) == 0)
	{
		return true;
	}
	size_t digits = 0;
	while (digits < text.size() && text[digits] >= '0' && text[digits] <= '9')
	{
		digits++;
	}
	const std::string rest = text.substr(digits);
	return digits > 0 && (rest == " Warning(s)" || rest == " Error(s)");
}

static void BuildNow(const std::string& dotnet, const std::string& csproj, const fs::path& projectFolder)
{
	const auto start = Clock::now();
	Log::GetLogger()->info("Scripts: building '{0}'", fs::u8path(csproj).filename().u8string());

	// Debug: the scripts are debugged in the editor. The engine's own ScriptCore (a code editor's build uses the csproj's)
	std::string scriptCore = Process::GetExecutablePath().parent_path().generic_u8string() + "/";
	const std::vector<std::string> arguments = { "build", csproj, "-c", "Debug", "--nologo", "-v", "q", "-tl:off",
		"-clp:NoSummary", "-p:GenerateFullPaths=true", "-p:MoravaScriptCoreDir=" + scriptCore };

	ScriptBuildResult result;
	std::set<std::string> seen; // MSBuild may say a message twice
	std::string error;
	const int exitCode = Process::Run(dotnet, arguments, std::string(), [&](const std::string& line)
	{
		if (line.find_first_not_of(" \t") == std::string::npos || !seen.insert(line).second)
		{
			return;
		}
		std::string text;
		bool isError = false;
		if (ReadCompilerLine(line, projectFolder, text, isError))
		{
			if (isError)
			{
				result.Errors++;
				Log::GetLogger()->error("{0}", text);
			}
			else
			{
				result.Warnings++;
				Log::GetLogger()->warn("{0}", text);
			}
			if (result.FirstMessage.empty() && isError)
			{
				result.FirstMessage = text;
			}
		}
		else if (line.find("error") != std::string::npos)
		{
			result.Errors++;
			Log::GetLogger()->error("Scripts: {0}", line); // MSBuild's own ("MSB1009: Project file does not exist")
			if (result.FirstMessage.empty())
			{
				result.FirstMessage = line;
			}
		}
		else if (!IsBuildSummaryLine(line))
		{
			Log::GetLogger()->info("Scripts: {0}", line);
		}
	}, error);

	result.Seconds = std::chrono::duration<double>(Clock::now() - start).count();
	result.Finished = true;
	if (exitCode < 0)
	{
		result.Succeeded = false;
		result.FirstMessage = error;
		Log::GetLogger()->error("Scripts: the build could not start: {0}", error);
	}
	else if (exitCode != 0)
	{
		result.Succeeded = false;
		if (result.Errors == 0)
		{
			result.Errors = 1;
		}
		Log::GetLogger()->error("Scripts: the build failed, {0} error(s) ({1:.1f} s): double-click an error in the Console to open it",
			result.Errors, result.Seconds);
	}
	else
	{
		result.Succeeded = true;
		Log::GetLogger()->info("Scripts: built in {0:.1f} s{1}", result.Seconds,
			result.Warnings ? ", " + std::to_string(result.Warnings) + " warning(s)" : std::string());
	}

	std::lock_guard<std::mutex> lock(s_ResultMutex);
	s_LastResult = result;
	s_WorkerDone = true;
}

// The newest .cs in the scripts' folder (not in bin/ and obj/, the build's)
static fs::file_time_type FindNewestScript(const fs::path& folder)
{
	fs::file_time_type newest = fs::file_time_type::min();
	std::error_code error;
	fs::recursive_directory_iterator it(folder, fs::directory_options::skip_permission_denied, error), end;
	for (; !error && it != end; it.increment(error))
	{
		const fs::path& path = it->path();
		const std::string name = path.filename().string();
		if (it->is_directory(error) && (name == "bin" || name == "obj" || (!name.empty() && name.front() == '.')))
		{
			it.disable_recursion_pending();
			continue;
		}
		if (path.extension() == ".cs" || path.extension() == ".csproj")
		{
			newest = std::max(newest, it->last_write_time(error));
		}
	}
	return newest;
}

static bool StartBuild()
{
	const std::string csproj = GetScriptProjectPath();
	const Project* project = Project::GetOpen();
	std::error_code error;
	if (csproj.empty() || !project)
	{
		ShowNotification("The project has no script project to build (Project Settings: Script Project)", true);
		return false;
	}
	if (!fs::is_regular_file(fs::u8path(csproj), error))
	{
		Log::GetLogger()->error("Scripts: the script project '{0}' doesn't exist", csproj);
		ShowNotification("The script project doesn't exist: the log says where it was looked for", true);
		return false;
	}
	const std::string dotnet = FindDotNet();
	if (dotnet.empty())
	{
		Log::GetLogger()->error("Scripts: no dotnet to build them with: install the .NET 10 SDK (https://dotnet.microsoft.com/download)");
		ShowNotification("Scripts can't be built: install the .NET 10 SDK (the log says more)", true);
		return false;
	}
	if (s_Worker.joinable())
	{
		s_Worker.join();
	}
	s_Building = true;
	s_WorkerDone = false;
	s_StartedAt = Clock::now();
	s_Worker = std::thread(BuildNow, dotnet, csproj, fs::u8path(project->GetDirectory()));
	return true;
}

void UpdateScriptBuild()
{
	// A finished build: its result where the user looks (the Console has the details)
	if (s_WorkerDone)
	{
		s_WorkerDone = false;
		s_Worker.join();
		s_Building = false;
		const ScriptBuildResult result = GetLastScriptBuild();
		if (!result.Succeeded)
		{
			ShowNotification("Scripts: " + (result.FirstMessage.empty() ? std::string("the build failed") : result.FirstMessage), true);
		}
	}

	const Clock::time_point now = Clock::now();
	const std::string csproj = GetScriptProjectPath();
	if (csproj.empty() || !ScriptHost::IsAvailable())
	{
		s_WatchedProject.clear();
		s_BuildRequested = false;
		return;
	}
	const fs::path folder = fs::u8path(csproj).parent_path();

	// Build on Save: a script newer than when it was last looked at. A project just opened builds when its assembly is
	// older than its scripts (or missing): what the project has is what the game runs
	if (now >= s_NextCheck)
	{
		s_NextCheck = now + s_CheckInterval;
		const fs::file_time_type newest = FindNewestScript(folder);
		if (s_WatchedProject != csproj)
		{
			s_WatchedProject = csproj;
			s_NewestScript = newest;
			s_ChangeSeen = false;
			std::error_code error;
			const std::string assembly = Project::GetOpen()->GetScriptAssemblyPath();
			const fs::file_time_type built = assembly.empty() ? fs::file_time_type::min() : fs::last_write_time(fs::u8path(assembly), error);
			if (s_BuildScriptsOnSave && newest != fs::file_time_type::min() && (error || built < newest))
			{
				Log::GetLogger()->info("Scripts: the assembly is older than the scripts (or not built yet): building them");
				s_BuildRequested = true;
			}
		}
		else if (newest != s_NewestScript)
		{
			s_NewestScript = newest;
			s_ChangeSeen = true;
			s_ChangedAt = now;
		}
	}
	if (s_ChangeSeen && s_BuildScriptsOnSave && now - s_ChangedAt >= s_SettleTime)
	{
		s_ChangeSeen = false;
		s_BuildRequested = true;
	}

	// One build at a time: a request during a build runs after it (the scripts may have changed again)
	if (s_BuildRequested && !s_Building)
	{
		s_BuildRequested = false;
		StartBuild();
	}
}

void ShutdownScriptBuild()
{
	if (s_Worker.joinable())
	{
		s_Worker.join(); // a build in progress finishes: dotnet left alone could be writing the assembly
	}
}
