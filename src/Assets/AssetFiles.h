#pragma once

#include "AssetPack.h"

#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>


/**
 * Where the engine reads its assets from (docs/engine/Game_Engine_Runtime_Guide.html, 10.3): the disk, or a mounted pack file.
 * Whoever loads an asset asks here with the path it always had, and doesn't know the difference. With a pack mounted the disk
 * isn't asked at all: what isn't in the pack doesn't exist, so a game can't depend on a file by accident.
 *
 * Only for a game's assets: a player's settings, logs and caches stay plain files.
 */
namespace AssetFiles
{
	// In the pack: the project's file, as Project::LoadFromPack reads it
	constexpr const char* PackProjectFile = "Game.mproject";
	constexpr const char* DefaultPackName = "Game.mpak";

	// Once, before anything loads. False (error says why): the file can't be read as a pack
	bool MountPack(const std::string& file, std::string& error);
	bool IsPackMounted();
	const AssetPack* GetPack();       // nullptr: none mounted
	const std::string& GetPackFile(); // absolute, forward slashes; empty: none mounted

	bool Exists(const std::string& path);
	// The file's bytes. False: it doesn't exist or can't be read (logged when it is damaged)
	bool Read(const std::string& path, std::vector<uint8_t>& bytes);
	bool ReadText(const std::string& path, std::string& text);
	// As YAML::LoadFile: throws when the file can't be read or parsed
	YAML::Node LoadYaml(const std::string& path);

	// With a pack mounted: a cooked entry of a file. False: the pack has none of this kind (or it is damaged: logged)
	bool ReadCooked(const std::string& path, AssetPack::Kind kind, std::vector<uint8_t>& bytes);

	// With a pack mounted: the last file asked for that it doesn't have or can't read, a hint for a crash that follows. May be
	// a file that was only tried. Empty: none
	std::string GetLastFailedFile();
	void NoteFailedFile(const std::string& path); // for a file the game can't go on without, found missing by who needs it
	// Called on the reading thread when an entry of the pack is damaged, after it is logged (a shipped game stops there)
	void SetDamagedFileHandler(const std::function<void(const std::string& file)>& handler);
}
