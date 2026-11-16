#pragma once

#include <cstdint>


/**
 * What a navmesh is baked for (docs/games/Shooter_Plan.html, E1): one agent size per navmesh, the character controller's
 * defaults, and the bake's grid. Meters and degrees; the engine's space (Y up), which is Recast's and Detour's too.
 */
struct NavMeshSettings
{
	float AgentRadius = 0.3f;      // how far from walls the mesh keeps
	float AgentHeight = 1.8f;      // the room under a ceiling it needs
	float AgentClimb = 0.35f;      // the step it walks up (a stair's riser, a curb)
	float MaxSlopeDegrees = 45.0f; // steeper ground isn't walkable

	float CellSize = 0.15f;        // the grid across (x and z): smaller is closer to the walls, slower to bake
	float CellHeight = 0.1f;       // the grid up (y)
	float TileSize = 32.0f;        // a tile's width in meters: a tile can be baked again on its own later

	float RegionMinArea = 4.0f;    // square meters: islands smaller than this are dropped (a table top, a crate's lid)
	float EdgeMaxError = 1.3f;     // how far (in cells) a polygon's edge may leave the walkable area's outline
	float DetailSampleDistance = 6.0f;   // in cells: the detail mesh's height samples (0: no detail mesh)
	float DetailSampleMaxError = 1.0f;   // in cells of height
};
