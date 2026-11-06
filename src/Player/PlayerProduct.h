#pragma once

#include <string>


struct Project;

/**
 * What makes a built game behave like a product on someone else's machine (docs/engine/Game_Engine_Runtime_Guide.html, 11.4): a
 * log and crash dumps in the user's folder, and a message in a sentence when it can't go on.
 *
 * The log and the crash handler are for a shipped game (Project::Shipped). A developer's runs keep the standard output and the
 * debugger's view of a crash.
 */
namespace PlayerProduct
{
	// Messages are shown in a dialog only to someone who started the game as a game: not when a test or a developer named what
	// to play, or set --exit-after (nobody there to press OK)
	void SetMessagesShown(bool shown);

	// The title of the messages and the folder they name, once the project is known
	void SetProject(const Project& project);

	// Tells the player why the game can't go on: in the log, and in a dialog when one can be seen
	void ShowFatalMessage(const std::string& message);

	// A shipped game's log: Player.log in the user's folder (the last run's kept as Player-previous.log), the info level unless
	// verbose. Without a folder it can write, it runs without the file
	void StartLogFile(const Project& project, bool verbose);

	// A crash from here on: a dump (Windows) and a line in the log, in the user's folder, and a message. Again after the .NET
	// runtime started: it sets its own filter
	void InstallCrashHandler(const Project& project);

	// A damaged entry of the pack: the game's files have to be installed again, it doesn't go on half loaded
	void StopOnDamagedFiles();

	// --crash-test: the check of all this. kind: abort, terminate, overflow, else a bad address
	[[noreturn]] void Crash(const std::string& kind);

	// At the normal end of the game: what is still in the log's buffer
	void FlushLog();
}
