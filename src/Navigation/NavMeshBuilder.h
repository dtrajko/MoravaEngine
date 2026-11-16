#pragma once

#include "Navigation/NavMesh.h"
#include "Navigation/NavMeshSettings.h"

#include <glm/glm.hpp>

#include <array>
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
		std::vector<uint8_t> NotWalkable; // per triangle (or none): 1, its top isn't walkable (a collider's Not Walkable)

		// A Nav Area's box (E1.5): its corners in the world. Blocked: no polygons in it (and the agent's radius around it);
		// else its polygons get Id (NavAreaIds)
		struct Area
		{
			glm::vec3 Corners[8];
			unsigned char Id = NavAreaIds::Ground;
			bool Blocked = false;
		};
		std::vector<Area> Areas;
		// Upright Box colliders' corners (ScenePhysics::GetStaticGeometry's solidBoxes): no floor inside them below their top
		// (the bake sees surfaces only: a box's hollow inside as tall as a person would be an island of its own)
		std::vector<std::array<glm::vec3, 8>> Solids;

		// A Nav Link (E1.5): Detour's off-mesh connection from Start to End (and back when Bidirectional), its ends joined
		// to the polygons within Radius; its polygon gets Area (NavAreaIds' links) and UserId (its index in LinkEntities)
		struct Link
		{
			glm::vec3 Start = glm::vec3(0.0f), End = glm::vec3(0.0f);
			bool Bidirectional = true;
			float Radius = 0.5f;
			unsigned char Area = NavAreaIds::WalkLink;
			uint32_t UserId = 0;
		};
		std::vector<Link> Links;
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

	// Bakes a room made in code (a floor, a wall across most of it, a raised platform, a crate, areas and a link) and checks
	// paths through it: around the wall, not through it, only partly onto the platform until a ladder leads up; a door
	// closed and opened, a Blocked area and a Not Walkable crate off the mesh. Logs "navigation self-test failed: ..." (CI
	// fails on it) or that it passed. In the editor, at its start
	bool SelfTest();
}
