#pragma once

#include <string>
#include <vector>


struct Project;

/**
 * Builds a game to ship (docs/engine/Game_Engine_Runtime_Guide.html, Phase 11): MoravaEngine --build-game <file.mproject>
 * [--build-output <folder>] [--build-platform <name>], which File > Build Game runs as a program of its own. Runs without a
 * window. No C++ is compiled:
 *
 *   1. The game's C# scripts: dotnet publish, Release
 *   2. Its pack: PackBuilder, with the scenes the project's Build settings list
 *   3. The player template: the Release player with what it needs beside it (the private .NET, ScriptCore, the license notices;
 *      on macOS the Vulkan loader and MoltenVK), copied and named after the product. Another platform's template is CI's,
 *      downloaded into PlayerTemplates/<platform> beside the editor (Tools/Get-PlayerTemplates.ps1)
 *   4. Per platform: on Windows the product's icon and version information in the executable; on Linux a .tar.gz of the folder
 *      beside it; on macOS an app (Name.app) with its Info.plist and icon
 *
 * The output (default: <Project>/Build/<platform>) runs by itself: nothing in it refers to the engine or the project.
 */
namespace GameBuilder
{
	// The platform this engine builds for, as .NET names it: win-x64, linux-x64, osx-arm64, osx-x64
	const char* GetPlatformName();

	// Where a project's game goes when no folder is named (platform empty: this one's)
	std::string GetDefaultOutputFolder(const Project& project, const std::string& platform = std::string());

	// The folder of other platforms' player templates: PlayerTemplates beside the editor
	std::string GetTemplatesFolder();

	// Whether this editor builds games for the platform (its own, and Linux from a template), and those it can now
	bool CanBuildFor(const std::string& platform);
	std::vector<std::string> GetPlatforms();

	// The .NET SDK's dotnet: on the PATH, in DOTNET_ROOT, else the one the engine was built with. Empty: none
	std::string FindDotNet();

	// The program's exit code: 0 when the game was built, 1 otherwise (each reason is logged)
	int Run(const std::string& projectFile, const std::string& outputFolder, const std::string& platform = std::string());
}
