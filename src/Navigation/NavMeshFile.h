#pragma once

#include "Navigation/NavMesh.h"

#include <string>


/**
 * A scene's baked navmesh, <Scene>.mnav next to its .mscene (docs/games/Shooter_Plan.html, E1): binary, the bake's settings,
 * the hash of what it was baked from (NavMeshData::SourceHash) and Detour's tiles as they are. Little-endian, as Detour's
 * tiles are on every platform the engine runs on. Read through AssetFiles (a game's pack carries it); written by the editor.
 *
 *   "MNAV", version (uint32), the settings (float each), SourceHash (uint64), Origin (3 floats), TileWorldSize (float),
 *   TilesX, TilesZ, MaxPolysPerTile, Polygons (int32 each), the tile count (uint32), then each tile: X, Z (int32), its size
 *   in bytes (uint32) and the bytes; then (version 2) the Nav Areas' entities and the Nav Links' (a count, uint32, and the
 *   IDs, uint64 each)
 */
namespace NavMeshFile
{
	constexpr const char* Extension = ".mnav";

	// The navmesh file of a scene file: the same path with the .mnav extension
	std::string GetPathForScene(const std::string& scenePath);

	// False (error says why) when it can't be written; written to a temporary file first, so a failed write leaves the old one
	bool Write(const std::string& path, const NavMeshData& data, std::string& error);
	// False (error says why) when it is missing, damaged or of another version
	bool Read(const std::string& path, NavMeshData& data, std::string& error);
}
