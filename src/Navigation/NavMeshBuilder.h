#pragma once

#include "Navigation/NavMesh.h"
#include "Navigation/NavMeshSettings.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>


/**
 * The editor's navmesh bake (Recast; docs/games/Shooter_Plan.html, E1): triangles in the world in, a tiled navmesh's
 * tiles out. Only the editor has it, as it has assimp: a player loads what was baked.
 */
namespace NavMeshBuilder
{
	// The geometry to walk on and around: triangles in the world (the scene's static colliders, E1.2)
	struct Input
	{
		std::vector<glm::vec3> Vertices;
		std::vector<uint32_t> Indices; // three per triangle
	};

	struct Result
	{
		glm::vec3 Origin = glm::vec3(0.0f);  // the baked area's lowest corner: tile (0, 0) starts there
		float TileWorldSize = 0.0f;          // a tile's width in meters (a whole number of cells)
		int TilesX = 0, TilesZ = 0;          // the grid of tiles over the area
		int MaxPolysPerTile = 0;
		std::vector<NavMesh::Tile> Tiles;    // the tiles that have something walkable
		int Polygons = 0;
		double Milliseconds = 0.0;
		std::string Error;                   // empty when it worked
	};

	Result Build(const Input& input, const NavMeshSettings& settings);

	// The baked tiles into a navmesh for queries; false (and an error logged) if Detour refuses them
	bool Load(const Result& result, const NavMeshSettings& settings, NavMesh& navMesh);

	// Bakes a room made in code (a floor, a wall across most of it, a raised platform) and checks paths through it:
	// around the wall, not through it, and only partly onto the platform. Logs "navigation self-test failed: ..." (CI
	// fails on it) or that it passed. In the editor, at its start
	bool SelfTest();
}
