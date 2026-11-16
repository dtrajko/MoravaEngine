#pragma once

#include "Navigation/NavMesh.h"
#include "Navigation/NavMeshSettings.h"

#include <glm/glm.hpp>

#include <atomic>
#include <string>
#include <vector>


/**
 * The editor's navmesh bake (Recast; docs/games/Shooter_Plan.html, E1): triangles in the world in, a tiled navmesh's
 * tiles out. Only the editor has it, as it has assimp: a player loads what was baked.
 */
namespace NavMeshBuilder
{
	// The geometry to walk on and around: triangles in the world (the scene's static colliders:
	// ScenePhysics::GetStaticGeometry)
	struct Input
	{
		std::vector<glm::vec3> Vertices;
		std::vector<uint32_t> Indices; // three per triangle
	};

	struct Result
	{
		NavMeshData Data;                    // its SourceHash is the caller's to set
		double Milliseconds = 0.0;
		std::string Error;                   // empty when it worked
		bool Canceled = false;
	};

	// How far a bake is, for a bake on another thread: the tiles done of all, and a request to stop
	struct Progress
	{
		std::atomic<int> TilesDone { 0 };
		std::atomic<int> TilesTotal { 0 };
		std::atomic<bool> Cancel { false };
	};

	// Bakes the tiles over the triangles' area (each tile from the triangles over it and its border). progress, when
	// given, is updated as tiles finish and read for a cancel (then Result.Canceled, no tiles)
	Result Build(const Input& input, const NavMeshSettings& settings, Progress* progress = nullptr);

	// Bakes a room made in code (a floor, a wall across most of it, a raised platform) and checks paths through it:
	// around the wall, not through it, and only partly onto the platform. Logs "navigation self-test failed: ..." (CI
	// fails on it) or that it passed. In the editor, at its start
	bool SelfTest();
}
