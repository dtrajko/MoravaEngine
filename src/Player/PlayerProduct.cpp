#include "PlayerProduct.h"

#include "Assets/AssetFiles.h"

#include "Core/Log.h"
#include "Core/Process.h"

#include "Game/GameSettings.h"

#include "H2M/Core/LogH2M.h"

#include "Project/Project.h"

#include "spdlog/sinks/basic_file_sink.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#else
#include <unistd.h>
#endif


namespace fs = std::filesystem;

namespace PlayerProduct
{
	static bool s_MessagesShown = false;
	static std::string s_Title; // empty until the project is known: then the executable's name
	static std::string s_Folder;  // the user's folder of this game; empty: not known yet
	static std::string s_Version = "0";

	void SetMessagesShown(bool shown)
	{
		s_MessagesShown = shown;
	}

	void SetProject(const Project& project)
	{
		s_Title = project.GetProductName();
		s_Version = project.Build.Version.empty() ? "0" : project.Build.Version;
		s_Folder = GameSettings::GetUserDataFolder(project);
	}

	static void ShowDialog(const std::string& message)
	{
		if (s_Title.empty())
		{
			s_Title = Process::GetExecutablePath().stem().u8string();
			std::replace(s_Title.begin(), s_Title.end(), '_', ' '); // Build Game's Coin_Collector is the product Coin Collector
		}
#if defined(_WIN32)
		const auto toWide = [](const std::string& text)
		{
			std::wstring wide(MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), (int)wide.size());
			return wide;
		};
		// A window in full screen would stay in front of the message. Async: the window's thread may be the one that crashed
		DWORD process = 0;
		HWND foreground = GetForegroundWindow();
		GetWindowThreadProcessId(foreground, &process);
		if (foreground && process == GetCurrentProcessId())
		{
			ShowWindowAsync(foreground, SW_HIDE);
		}
		MessageBoxW(nullptr, toWide(message).c_str(), toWide(s_Title).c_str(), MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
#else
		std::string error;
		const auto ignore = [](const std::string&) {};
#if defined(__APPLE__)
		// The texts go in as arguments of the script: nothing in them can be read as AppleScript
		Process::Run("/usr/bin/osascript", { "-e", "on run {title, message}", "-e", "display alert title message message as critical", "-e", "end run", s_Title, message },
			std::string(), ignore, error);
#else
		// Linux has no dialog of its own: what the desktop brings, else the log has it
		std::string program = Process::FindOnPath("zenity");
		if (!program.empty())
		{
			Process::Run(program, { "--error", "--no-markup", "--title", s_Title, "--text", message }, std::string(), ignore, error);
		}
		else if (!(program = Process::FindOnPath("kdialog")).empty())
		{
			Process::Run(program, { "--title", s_Title, "--error", message }, std::string(), ignore, error);
		}
#endif
#endif
	}

	void FlushLog()
	{
		for (const std::shared_ptr<spdlog::logger>& logger : { Log::GetCoreLogger(), Log::GetClientLogger(), H2M::LogH2M::GetCoreLogger(), H2M::LogH2M::GetClientLogger() })
		{
			logger->flush();
		}
	}

	void ShowFatalMessage(const std::string& message)
	{
		Log::GetLogger()->error("{0}", message);
		FlushLog();
		if (s_MessagesShown)
		{
			ShowDialog(message);
		}
	}

	void StartLogFile(const Project& project, bool verbose)
	{
		const spdlog::level::level_enum level = verbose ? spdlog::level::trace : spdlog::level::info;
		const std::vector<std::shared_ptr<spdlog::logger>> loggers = { Log::GetCoreLogger(), Log::GetClientLogger(),
			H2M::LogH2M::GetCoreLogger(), H2M::LogH2M::GetClientLogger() };
		for (const std::shared_ptr<spdlog::logger>& logger : loggers)
		{
			logger->set_level(level);
		}

		const std::string folder = GameSettings::GetUserDataFolder(project);
		if (folder.empty())
		{
			return;
		}
		std::error_code error;
		const fs::path path = fs::u8path(folder);
		fs::create_directories(path, error);
		const fs::path file = path / "Player.log";
		fs::rename(file, path / "Player-previous.log", error); // no file yet, or another copy of the game has it open: not a problem

		try
		{
			auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file.u8string(), true);
			sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %n: %v");
			for (const std::shared_ptr<spdlog::logger>& logger : loggers)
			{
				logger->sinks().push_back(sink);
				logger->flush_on(spdlog::level::warn); // each line on a slow disk would hold up the game; the rest every two seconds
			}
			spdlog::flush_every(std::chrono::seconds(2));
			Log::GetLogger()->info("{0} {1}: log started in '{2}'", project.GetProductName(), project.Build.Version, folder);
		}
		catch (const spdlog::spdlog_ex& exception)
		{
			Log::GetLogger()->warn("The log file '{0}' can't be written: {1}", file.u8string(), exception.what());
		}
	}

	// A line for the log. On Windows on another thread and with a time limit: the game may have crashed holding the log's lock
	static void LogCrash(const std::string& what)
	{
		Log::GetLogger()->critical("The game crashed: {0}", what);
		const std::string failed = AssetFiles::GetLastFailedFile();
		if (!failed.empty())
		{
			Log::GetLogger()->critical("The last file it could not read: '{0}'", failed);
		}
		FlushLog();
	}

#if defined(_WIN32)
	static const int KeptDumps = 5;
	static const DWORD FatalCode = 0xE0A00001; // not from the system: abort, terminate and the like, sent to the filter below
	static std::atomic_flag s_Crashed = ATOMIC_FLAG_INIT;

	struct CrashState
	{
		EXCEPTION_POINTERS* Exception = nullptr;
		DWORD ThreadId = 0;
		std::string DumpName;
		std::string Message;
	};

	static std::string MakeCrashMessage(const std::string& dumpName)
	{
		std::string message = s_Title + " has stopped because of an error.";
		const std::string failed = AssetFiles::GetLastFailedFile();
		if (!failed.empty())
		{
			message += "\n\nThe game could not read '" + failed + "'. Installing the game again may help.";
		}
		if (!s_Folder.empty())
		{
			message += "\n\nThe log" + std::string(dumpName.empty() ? " is" : " and the crash report are") + " in:\n" + fs::u8path(s_Folder).make_preferred().u8string();
		}
		return message;
	}

	static DWORD WINAPI WriteDump(void* parameter)
	{
		CrashState& state = *static_cast<CrashState*>(parameter);
		if (s_Folder.empty())
		{
			return 1;
		}
		std::time_t now = std::time(nullptr);
		std::tm local;
		localtime_s(&local, &now);
		char stamp[32];
		strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &local);
		const std::string name = "Crash_" + s_Version + "_" + stamp + ".dmp";

		std::error_code error;
		fs::create_directories(fs::u8path(s_Folder), error);
		const fs::path file = fs::u8path(s_Folder) / fs::u8path(name);
		HANDLE handle = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return 1;
		}
		MINIDUMP_EXCEPTION_INFORMATION exception = {};
		exception.ThreadId = state.ThreadId;
		exception.ExceptionPointers = state.Exception;
		const MINIDUMP_TYPE type = static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
		const BOOL written = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), handle, type, &exception, nullptr, nullptr);
		CloseHandle(handle);
		if (written)
		{
			state.DumpName = name;
		}
		else
		{
			fs::remove(file, error);
		}
		return 0;
	}

	static DWORD WINAPI WriteCrashLog(void* parameter)
	{
		CrashState& state = *static_cast<CrashState*>(parameter);
		char code[64];
		snprintf(code, sizeof(code), "exception 0x%08lX", state.Exception->ExceptionRecord->ExceptionCode);
		LogCrash(std::string(code) + (state.DumpName.empty() ? ", no crash report written" : ", crash report " + state.DumpName));
		return 0;
	}

	static DWORD WINAPI ShowCrashDialog(void* parameter)
	{
		ShowDialog(static_cast<CrashState*>(parameter)->Message);
		return 0;
	}

	// On a thread of its own: a stack overflow leaves the crash's thread no room to work, and what crashed may hold a lock
	static void RunAndWait(LPTHREAD_START_ROUTINE work, CrashState& state, DWORD milliseconds)
	{
		HANDLE thread = CreateThread(nullptr, 256 * 1024, work, &state, 0, nullptr);
		if (thread)
		{
			WaitForSingleObject(thread, milliseconds);
			CloseHandle(thread);
		}
	}

	static LONG WINAPI OnCrash(EXCEPTION_POINTERS* exception)
	{
		if (s_Crashed.test_and_set())
		{
			Sleep(INFINITE); // a second thread: the first one finishes the job and ends the process
		}
		CrashState state;
		state.Exception = exception;
		state.ThreadId = GetCurrentThreadId();
		RunAndWait(WriteDump, state, 60000);
		RunAndWait(WriteCrashLog, state, 3000);
		if (s_MessagesShown)
		{
			state.Message = MakeCrashMessage(state.DumpName);
			RunAndWait(ShowCrashDialog, state, INFINITE);
		}
		TerminateProcess(GetCurrentProcess(), exception->ExceptionRecord->ExceptionCode); // the job is done on all threads' behalf
		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void RaiseFatal()
	{
		RaiseException(FatalCode, EXCEPTION_NONCONTINUABLE, 0, nullptr);
		TerminateProcess(GetCurrentProcess(), FatalCode); // when a handler on the way swallowed it
	}

	static void RemoveOldDumps()
	{
		std::vector<fs::path> dumps;
		std::error_code error;
		for (const fs::directory_entry& entry : fs::directory_iterator(fs::u8path(s_Folder), error))
		{
			if (entry.path().filename().u8string().rfind("Crash_", 0) == 0 && entry.path().extension() == ".dmp")
			{
				dumps.push_back(entry.path());
			}
		}
		std::sort(dumps.begin(), dumps.end(), [](const fs::path& a, const fs::path& b)
		{
			std::error_code ignored;
			return fs::last_write_time(a, ignored) > fs::last_write_time(b, ignored);
		});
		for (size_t i = KeptDumps; i < dumps.size(); i++)
		{
			fs::remove(dumps[i], error);
		}
	}

	void InstallCrashHandler(const Project& project)
	{
		SetProject(project);
		static bool s_Once = false;
		if (!s_Once)
		{
			s_Once = true;
			if (!s_Folder.empty())
			{
				RemoveOldDumps();
			}
			// What the C runtime and C++ end with goes to the filter too. Their own reports would be dialogs for a developer
			_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
			std::signal(SIGABRT, [](int) { RaiseFatal(); });
			std::set_terminate([]() { RaiseFatal(); });
			_set_purecall_handler([]() { RaiseFatal(); });
			_set_invalid_parameter_handler([](const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) { RaiseFatal(); });
		}
		SetUnhandledExceptionFilter(OnCrash);
	}
#else
	static volatile sig_atomic_t s_Crashed = 0;

	// No dumps here (the system's core dumps are there for that): a line in the log. Logging from a signal handler isn't safe in
	// general; the process is lost anyway, and the alarm ends it if the log's lock is taken
	static void OnCrash(int number)
	{
		if (!s_Crashed)
		{
			s_Crashed = 1;
			alarm(3);
			LogCrash("signal " + std::to_string(number));
		}
		std::signal(number, SIG_DFL);
		raise(number);
	}

	void InstallCrashHandler(const Project& project)
	{
		SetProject(project);
		for (int number : { SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL })
		{
			std::signal(number, OnCrash);
		}
	}
#endif

	void StopOnDamagedFiles()
	{
		AssetFiles::SetDamagedFileHandler([](const std::string& file)
		{
			ShowFatalMessage("The game's files are damaged ('" + file + "' can't be read). Please install the game again.");
#if defined(_WIN32)
			TerminateProcess(GetCurrentProcess(), 1); // from any thread, and without what the half-loaded game would do on its way out
#else
			_exit(1);
#endif
		});
	}

	static int Overflow(volatile int depth)
	{
		volatile char room[4096] = {};
		room[depth % 4096] = 1;
		return depth < 0 ? 0 : Overflow(depth + 1) + room[0]; // (never below 0: a compiler refuses a recursion that can't end)
	}

	void Crash(const std::string& kind)
	{
		Log::GetLogger()->info("Crashing (--crash-test {0})", kind);
		if (kind == "abort")
		{
			std::abort();
		}
		if (kind == "terminate")
		{
			std::terminate();
		}
		if (kind == "overflow")
		{
			Overflow(0);
		}
		volatile int* nothing = nullptr;
		*nothing = 1;
		std::abort();
	}
}
