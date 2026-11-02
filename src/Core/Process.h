#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>


/**
 * Other programs, started from the engine: the .NET SDK that builds a project's scripts, the code editor that opens them
 * (docs/engine/Game_Engine_Runtime_Guide.html, 4.7). Paths and arguments are UTF-8; an argument is passed as it is (no
 * shell in between, except for a Windows .cmd or .bat file, which only cmd.exe runs).
 */
namespace Process
{
	// Runs a program to its end; each line it writes (its output and errors, as they come) goes to onLine, on the calling
	// thread. The exit code, or -1 when it can't be started (error says why). workingDirectory empty: the engine's
	int Run(const std::string& program, const std::vector<std::string>& arguments, const std::string& workingDirectory,
		const std::function<void(const std::string& line)>& onLine, std::string& error);

	// Starts a program and doesn't wait for it (a code editor). False (error says why) when it can't be started
	bool Start(const std::string& program, const std::vector<std::string>& arguments, std::string& error);

	// Opens a file or folder with the program the system has for it (Explorer, Finder, xdg-open)
	bool Open(const std::string& path, std::string& error);

	// A program on the PATH, absolute (on Windows also name.exe, name.cmd and name.bat); empty: none
	std::string FindOnPath(const std::string& name);

	// The running executable, absolute (its folder holds Morava.ScriptCore.dll and the private dotnet/)
	std::filesystem::path GetExecutablePath();
}
