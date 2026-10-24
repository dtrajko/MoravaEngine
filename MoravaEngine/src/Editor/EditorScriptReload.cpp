// Hot reload of the project's C# scripts (docs/engine/Game_Engine_Runtime_Guide.html, 4.6): the editor watches the
// project's script assembly (Project Settings: Script Assembly). A newer build is loaded once the file has stopped
// changing (dotnet build writes the DLL and its PDB), while the game isn't running: the old assembly is unloaded first and
// must be collected (ScriptHost reports what still holds it), then the new one is read from bytes and its classes and
// fields replace the old ones in the Properties panel. A build during Play waits for Stop; the game keeps the scripts it
// started with. The player doesn't reload: a shipped game's scripts don't change.


#include "Game/GameSystems.h"
#include "Project/Project.h"
#include "Scripting/ScriptHost.h"

#include "Core/Log.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>


#include "EditorState.h"


namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

static constexpr auto s_CheckInterval = std::chrono::milliseconds(250);
static constexpr auto s_SettleTime = std::chrono::milliseconds(500); // unchanged this long: the build has finished writing

static Clock::time_point s_NextCheck;
static bool s_ReloadRequested = false;  // Project panel: Reload
// The newest build seen, and since when it is unchanged
static bool s_BuildSeen = false;
static fs::file_time_type s_SeenTime;
static uintmax_t s_SeenSize = 0;
static Clock::time_point s_SeenSince;
// A build that couldn't be loaded is tried again only when there is a newer one
static bool s_Failed = false;
static fs::file_time_type s_FailedTime;
static bool s_WaitingForStop = false;   // told once: a build during Play is loaded at Stop
static std::vector<ScriptReloadRecord> s_History;

const std::vector<ScriptReloadRecord>& GetScriptReloadHistory()
{
	return s_History;
}

// A reload for the history (the Scripts panel)
static void Record(bool loaded, bool leaked, bool rebuilt, double milliseconds)
{
	ScriptReloadRecord record;
	const std::time_t now = std::time(nullptr);
	std::tm local {};
#if defined(_WIN32)
	localtime_s(&local, &now);
#else
	localtime_r(&now, &local);
#endif
	std::strftime(record.Time, sizeof(record.Time), "%H:%M:%S", &local);
	record.Loaded = loaded;
	record.Leaked = leaked;
	record.Rebuilt = rebuilt;
	record.Milliseconds = milliseconds;
	record.ClassCount = loaded ? ScriptHost::GetClasses().size() : 0;
	s_History.push_back(record);
}

void RequestScriptReload()
{
	s_ReloadRequested = true;
}

static void Reload(const std::string& path, fs::file_time_type writeTime, bool requested)
{
	const auto start = Clock::now();
	const uint32_t leaksBefore = ScriptHost::GetLeakedAssemblyCount();
	GameSystems::LoadScripts();
	const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();

	const bool loaded = ScriptHost::IsGameAssemblyLoaded() && ScriptHost::GetGameAssemblyPath() == fs::u8path(path);
	if (!loaded)
	{
		s_Failed = true;
		s_FailedTime = writeTime;
		Record(false, false, !requested, (double)milliseconds);
		ShowNotification("The scripts could not be loaded: the log says why", true);
		return;
	}
	s_Failed = false;
	Record(true, ScriptHost::GetLeakedAssemblyCount() != leaksBefore, !requested, (double)milliseconds);
	if (ScriptHost::GetLeakedAssemblyCount() != leaksBefore)
	{
		ShowNotification("Scripts reloaded, but the old ones are still held (a leak): the log says more", true);
	}
	else
	{
		ShowNotification("Scripts " + std::string(requested ? "reloaded" : "rebuilt and reloaded") + ": " +
			std::to_string(ScriptHost::GetClasses().size()) + " script class(es)");
	}
	Log::GetLogger()->info("Scripts: reloaded in {0} ms", milliseconds);
}

void UpdateScriptReload()
{
	const Clock::time_point now = Clock::now();
	if (!ScriptHost::IsAvailable() || (now < s_NextCheck && !s_ReloadRequested))
	{
		return;
	}
	s_NextCheck = now + s_CheckInterval;

	const Project* project = Project::GetOpen();
	const std::string path = project ? project->GetScriptAssemblyPath() : std::string();
	std::error_code error;
	const fs::path file = fs::u8path(path);
	const fs::file_time_type writeTime = path.empty() ? fs::file_time_type() : fs::last_write_time(file, error);
	const uintmax_t size = path.empty() || error ? 0 : fs::file_size(file, error);
	if (path.empty() || error)
	{
		s_ReloadRequested = false;
		s_BuildSeen = false;
		return; // no scripts, or the file is missing (not built yet, or being written)
	}

	if (s_ReloadRequested)
	{
		s_ReloadRequested = false;
		if (!IsPlaying())
		{
			s_BuildSeen = false;
			Reload(path, writeTime, true);
		}
		return;
	}

	// The loaded build, or one that failed: nothing new
	const bool loadedThis = ScriptHost::IsGameAssemblyLoaded() && ScriptHost::GetGameAssemblyPath() == file;
	if ((loadedThis && writeTime == ScriptHost::GetGameAssemblyWriteTime()) || (s_Failed && writeTime == s_FailedTime))
	{
		s_BuildSeen = false;
		s_WaitingForStop = false;
		return;
	}

	// A new build (or the first one): loaded once it has stopped changing
	if (!s_BuildSeen || writeTime != s_SeenTime || size != s_SeenSize)
	{
		s_BuildSeen = true;
		s_SeenTime = writeTime;
		s_SeenSize = size;
		s_SeenSince = now;
		return;
	}
	if (now - s_SeenSince < s_SettleTime)
	{
		return;
	}
	if (IsPlaying())
	{
		if (!s_WaitingForStop)
		{
			s_WaitingForStop = true;
			Log::GetLogger()->info("Scripts: '{0}' was rebuilt: it is loaded at Stop (the game keeps the scripts it started with)", path);
			ShowNotification("The scripts were rebuilt: they are reloaded at Stop");
		}
		return;
	}
	s_BuildSeen = false;
	s_WaitingForStop = false;
	Reload(path, writeTime, false);
}
