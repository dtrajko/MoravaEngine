#pragma once

#include <cstdint>
#include <string>
#include <vector>


namespace YAML
{
	class Emitter;
	class Node;
}

using AssetID = uint64_t; // 0: none

enum class AssetType : uint16_t { None, Model, Texture, EnvironmentMap, Sound, Font, Prefab, Material, Animator, RenderTexture, Scene };

const char* GetAssetTypeName(AssetType type);

/**
 * Which file is which asset (docs/engine/Game_Engine_Runtime_Guide.html, 10.1). An asset has a lasting ID, kept beside it in
 * a sidecar: Models/cube.obj has Models/cube.obj.meta. Moved together with its sidecar, in Explorer, Finder or Git, it is
 * still the same asset, and whatever refers to its ID finds it. A material has its ID in its own file, and no sidecar.
 *
 * The registry walks the project's asset and scenes folders and the engine's asset folders. The editor makes the missing
 * sidecars of the project's files, and of an engine file when something saved refers to it (the engine has far more files
 * than a game uses). The player only reads: a file without a sidecar is still found by its path.
 */
namespace AssetRegistry
{
	struct Record
	{
		AssetID ID = 0;
		AssetType Type = AssetType::None;
		std::string File;       // absolute, forward slashes
		std::string MemoryPath; // as the engine keeps paths (ProjectPaths::Normalize); made when first asked for
	};

	// True in the editor: sidecars may be written
	void SetWritable(bool writable);
	// A project was opened or closed: the next question walks the folders again
	void Invalidate();
	// Walks the folders again: files moved, added or removed since the last time
	void Refresh();

	// The ID of a file. Zero: it has none (not an asset's type, doesn't exist, no sidecar in the player, or outside the
	// project and the engine). A file moved since it was opened still has its ID by its old path.
	AssetID GetID(const std::string& path);
	// Where an asset is now, as the engine keeps paths. Empty: not known, or the file is gone
	std::string FindPath(AssetID id);

	// For the Assets panel
	const std::vector<Record>& GetRecords();               // sorted by path
	const std::vector<std::string>& GetOrphanedSidecars(); // a .meta whose file isn't beside it
	const std::vector<std::string>& GetDuplicates();       // files with an ID another file has (materials: not changed)
	double GetLastScanMilliseconds();
}

/**
 * A reference to an asset in a saved file: the path a person can read and that is used when the ID isn't known, and the ID,
 * which wins:
 *
 *   File: Models/Primitives/cube.obj
 *   FileID: 7310463185520941001
 */
namespace AssetReference
{
	enum class TextMode
	{
		Full,   // path and ID, as saved
		Stable, // the ID only (the path when there is none): the same text wherever the file moves. A prefab instance's hash
		Legacy  // the path only: the text as it was before assets had IDs, to check the hashes in older scenes
	};

	TextMode GetTextMode();

	struct ScopedTextMode
	{
		explicit ScopedTextMode(TextMode mode);
		~ScopedTextMode();
		TextMode Previous;
	};

	// Writes the key and the ID as <key>ID
	void Write(YAML::Emitter& out, const char* key, const std::string& path);
	// The file to open (as the engine keeps paths): by the ID if it is known and the file is there, else by the path. Empty:
	// the node has neither. idKey: when it isn't <key>ID (a material's ID)
	std::string Read(const YAML::Node& node, const char* key, const char* idKey = nullptr);
}
