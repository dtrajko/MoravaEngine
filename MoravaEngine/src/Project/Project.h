#pragma once

#include <cstdint>
#include <string>


// The window a project's game runs in (the player's; the editor keeps its own)
struct ProjectWindow
{
	uint32_t Width = 1280, Height = 720;
	bool Fullscreen = false; // the primary monitor, at its resolution
	bool VSync = true;
	std::string Title;       // empty: the project's name
};

/**
 * A game project (docs/engine/Game_Engine_Runtime_Guide.html, 1.6): a folder of its own, anywhere (a project does not live
 * in the engine's repository), with a .mproject file (YAML) that names the game, the scene it starts with and its window:
 *
 *   Project:
 *     Version: 1
 *     Name: RollABall
 *     StartScene: Scenes/Main.mscene
 *     AssetDirectory: Assets
 *     Window: { Width: 1600, Height: 900, Fullscreen: false, VSync: true, Title: Roll a Ball }
 *
 * The script assembly (Phase 4) and the input map (Phase 2) come with their phases.
 *
 * One project is open at a time (GetOpen): the paths in scene and material files resolve against it (ProjectPaths).
 */
struct Project
{
	static constexpr const char* FileExtension = ".mproject";
	static constexpr const char* ScenesFolder = "Scenes";       // in the project folder: new projects get it
	static constexpr const char* MaterialsFolder = "Materials"; // in the asset folder

	std::string Name = "Untitled";
	std::string StartScene;                // relative to the project folder (empty: none)
	std::string AssetDirectory = "Assets"; // relative to the project folder
	ProjectWindow Window;

	std::string FilePath; // the .mproject file, absolute (not written in it)

	// The project folder (the .mproject file's), the asset folder and the scenes folder: absolute, forward slashes
	std::string GetDirectory() const;
	std::string GetAssetFolder() const;
	std::string GetScenesFolder() const;
	// The window's title: Window.Title, or the name
	std::string GetWindowTitle() const;

	// Reads a project file. False (error explains) when it can't be read
	static bool Load(const std::string& filepath, Project& project, std::string& error);
	// Writes the project to its FilePath. False (error explains) when it can't be written
	bool Save(std::string& error) const;
	// A new project in a folder (made if it doesn't exist): <folder>/<folder's name>.mproject, named after the folder, with
	// its asset and scenes folders. False (error explains) when the folder has a project already or can't be written
	static bool Create(const std::string& folder, Project& project, std::string& error);

	// The open project (nullptr: none; the engine's own files only)
	static Project* GetOpen();
	static void SetOpen(const Project& project);
	static void Close();
};

/**
 * The paths in scene and material files. In a file, a path is relative to the open project's folder for the project's
 * files, relative to the working directory (the engine's folder) for the engine's own (its models, environment maps,
 * materials), and absolute for anything else. In memory, a path is one that opens the file: the project's files absolute,
 * the engine's relative to the working directory. Without a project, both are the same: relative to the working directory.
 */
namespace ProjectPaths
{
	// A path as written in a file, with forward slashes
	std::string ToStored(const std::string& path);
	// A path read from a file, to open: in the project folder when the file is there, else as it is (the engine's)
	std::string Resolve(const std::string& storedPath);
	// A path to keep in memory: Resolve(ToStored(path)), so the same file always has the same path
	std::string Normalize(const std::string& path);
}
