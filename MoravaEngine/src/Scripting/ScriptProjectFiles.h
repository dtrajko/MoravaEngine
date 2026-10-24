#pragma once

#include <filesystem>
#include <string>


struct Project;

/**
 * The files of a project's C# scripts that the engine writes (docs/engine/Game_Engine_Runtime_Guide.html, 4.7): the
 * scripts' own .NET project for a new game project, and a new script from a template. Nothing is overwritten.
 */
namespace ScriptProjectFiles
{
	// A C# name made of a game's or a script's name: "Roll a Ball" is RollABall, "2D Game" _2DGame (letters, digits and _;
	// what isn't one starts a new word). fallback when nothing is left
	std::string MakeIdentifier(const std::string& name, const std::string& fallback);
	bool IsIdentifier(const std::string& name);

	// Scripts/<Name>.csproj in the project's folder (Name: MakeIdentifier of the project's name), referencing
	// Morava.ScriptCore.dll in scriptCoreDirectory (the engine's executables' folder; the editor passes its own when it
	// builds), with its .gitignore (bin/, obj/), and the configurations that start the editor or the player with the project
	// under the debugger: Properties/launchSettings.json (Visual Studio, Rider) and .vscode (VS Code, also attach to the
	// editor), started in engineDirectory (the folder with Resources). Sets the project's ScriptProject and ScriptAssembly
	// (Scripts/bin/<Name>.dll); the caller saves the project. False (error says why) when a file can't be written
	bool CreateScriptProject(Project& project, const std::filesystem::path& scriptCoreDirectory,
		const std::filesystem::path& engineDirectory, std::string& error);

	// A script, <folder>/<className>.cs: a class deriving from Morava.Entity in namespaceName, with OnCreate and OnUpdate.
	// False (error says why) when the file exists or can't be written
	bool CreateScript(const std::filesystem::path& folder, const std::string& namespaceName, const std::string& className,
		std::filesystem::path& path, std::string& error);
}
