#pragma once

#include <string>


struct Project;

/**
 * Builds a game to ship (docs/engine/Game_Engine_Runtime_Guide.html, Phase 11): MoravaEngine --build-game <file.mproject>
 * [--build-output <folder>], which File > Build Game runs as a program of its own. Runs without a window. No C++ is compiled:
 *
 *   1. The game's C# scripts: dotnet publish, Release
 *   2. Its pack: PackBuilder, with the scenes the project's Build settings list
 *   3. The player template: the Release player with what it needs beside it (the private .NET, ScriptCore, the license notices),
 *      copied and named after the product
 *   4. On Windows, the product's icon and version information in the executable
 *
 * The output folder (default: <Project>/Build/<platform>) runs by itself: nothing in it refers to the engine or the project.
 */
namespace GameBuilder
{
	// The platform this engine builds for, as .NET names it: win-x64, linux-x64, osx-arm64, osx-x64
	const char* GetPlatformName();

	// Where a project's game goes when no folder is named
	std::string GetDefaultOutputFolder(const Project& project);

	// The .NET SDK's dotnet: on the PATH, in DOTNET_ROOT, else the one the engine was built with. Empty: none
	std::string FindDotNet();

	// The program's exit code: 0 when the game was built, 1 otherwise (each reason is logged)
	int Run(const std::string& projectFile, const std::string& outputFolder);
}
