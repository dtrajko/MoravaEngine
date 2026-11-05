#pragma once

#include "Audio/AudioBus.h"
#include "Physics/PhysicsCollisionMatrix.h"

#include <cstdint>
#include <string>
#include <vector>


namespace YAML { class Node; }

// The window a project's game runs in (the player's; the editor keeps its own)
struct ProjectWindow
{
	uint32_t Width = 1280, Height = 720;
	bool Fullscreen = false; // the primary monitor, at its resolution
	bool VSync = true;
	std::string Title;       // empty: the project's name
};

// A scene a game ships, as saved: its path and its ID (AssetReference)
struct ProjectBuildScene
{
	std::string File;
	uint64_t ID = 0;
};

// What File > Build Game makes of the project (Phase 11)
struct ProjectBuild
{
	std::string ProductName;               // empty: the project's name
	std::string Company;
	std::string Version = "1.0.0";         // up to four numbers
	std::string Icon;                      // a square image, relative to the project folder (empty: none)
	std::vector<ProjectBuildScene> Scenes; // the scenes it ships (empty: all the project's); the start scene always
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
 *     InputMap: Settings/Input.minput
 *     ScriptAssembly: Scripts/bin/RollABall.dll
 *     ScriptProject: Scripts/RollABall.csproj
 *     Window: { Width: 1600, Height: 900, Fullscreen: false, VSync: true, Title: Roll a Ball }
 *     Physics: { Collisions: [Static-Dynamic, Static-Character, Dynamic-Dynamic, ...] }
 *     Audio: { Master: 1, Music: 0.8, Effects: 1, UI: 1 }
 *     Build: { ProductName: Roll a Ball, Company: Morava Games, Version: 1.0.0, Icon: Assets/Icon.png, Scenes: [{File: Scenes/Main.mscene, FileID: 123}] }
 *
 * One project is open at a time (GetOpen): the paths in scene and material files resolve against it (ProjectPaths).
 */
struct Project
{
	static constexpr const char* FileExtension = ".mproject";
	static constexpr const char* ScenesFolder = "Scenes";       // in the project folder: new projects get it
	static constexpr const char* MaterialsFolder = "Materials"; // in the asset folder
	static constexpr const char* DefaultInputMap = "Settings/Input.minput";
	static constexpr const char* BuildFolder = "Build";         // in the project folder: where a pack is built (PackBuilder)

	std::string Name = "Untitled";
	std::string StartScene;                // relative to the project folder (empty: none)
	std::string AssetDirectory = "Assets"; // relative to the project folder
	std::string InputMapFile = DefaultInputMap; // the actions and their bindings (InputMap), relative to the project folder
	std::string ScriptAssembly;            // the game's C# scripts as built, relative to the project folder (empty: none)
	std::string ScriptProject;             // the scripts' .NET project (.csproj) the editor builds, relative to the project folder (empty: none)
	ProjectWindow Window;
	PhysicsCollisionMatrix Collisions; // the physics layers that collide (written as the pairs that do)
	AudioVolumes Volumes;              // the master's and the buses' (the Audio panel's mixer)
	ProjectBuild Build;
	bool Shipped = false;              // the project in a pack made by Build Game: a product, not work in progress. Never in a .mproject

	std::string FilePath; // the .mproject file, absolute (not written in it)

	// The project folder (the .mproject file's), the asset folder and the scenes folder: absolute, forward slashes
	std::string GetDirectory() const;
	std::string GetAssetFolder() const;
	std::string GetScenesFolder() const;
	std::string GetInputMapPath() const; // empty: the project has none (the game gets InputMap::Default)
	std::string GetScriptAssemblyPath() const; // empty: the project has no scripts
	std::string GetScriptProjectPath() const;  // empty: the project has no scripts to build
	// The window's title: Window.Title, or the product's name
	std::string GetWindowTitle() const;
	std::string GetProductName() const; // Build.ProductName, or the name
	std::string GetIconPath() const;    // empty: the project has no icon
	// The scenes of Build.Scenes as kept paths, found by their IDs. Asks the open project and the registry: this project must be open
	std::vector<std::string> GetBuildScenes() const;

	// Reads a project file. False (error explains) when it can't be read
	static bool Load(const std::string& filepath, Project& project, std::string& error);
	// The project of a mounted pack: its folder is the pack's, where the game's script assembly is
	static bool LoadFromPack(Project& project, std::string& error);
	// Writes the project to its FilePath. False (error explains) when it can't be written
	bool Save(std::string& error) const;
	// A new project in a folder (made if it doesn't exist): <folder>/<folder's name>.mproject, named after the folder, with
	// its asset and scenes folders and the default input file (InputMap::Default). False (error explains) when the folder
	// has a project already or can't be written
	static bool Create(const std::string& folder, Project& project, std::string& error);

	static bool FromYaml(const YAML::Node& root, const std::string& filepath, Project& project, std::string& error);

	// The open project (nullptr: none; the engine's own files only)
	static Project* GetOpen();
	static void SetOpen(const Project& project);
	static void Close();
};

/**
 * The paths in scene and material files. In a file, a path is relative to the open project's folder for the project's
 * files, relative to the working directory (the repository's root) for the engine's own (its models, environment maps,
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
