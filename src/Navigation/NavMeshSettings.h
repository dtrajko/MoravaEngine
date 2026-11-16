#pragma once

#include <cstddef>
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

	// The grid across (x and z): smaller is closer to the walls, slower to bake. A third of the agent's radius (Recast's
	// advice): with 0.15 m a stair's 28 cm tread is sometimes one cell, a step up on both sides, and the bake takes it for
	// too steep (the greybox's stairs broke halfway, 2026-11-16)
	float CellSize = 0.1f;
	float CellHeight = 0.05f;      // the grid up (y): half the cell size; the step height is rounded down to it
	float TileSize = 32.0f;        // a tile's width in meters: a tile can be baked again on its own later

	float RegionMinArea = 4.0f;    // square meters: islands smaller than this are dropped (a table top, a crate's lid)
	float EdgeMaxError = 1.3f;     // how far (in cells) a polygon's edge may leave the walkable area's outline
	float DetailSampleDistance = 6.0f;   // in cells: the detail mesh's height samples (0: no detail mesh)
	float DetailSampleMaxError = 1.0f;   // in cells of height

	// The values' hash (FNV-1a): a navmesh baked with other settings is out of date
	uint64_t GetHash() const
	{
		uint64_t hash = 14695981039346656037ull;
		const float values[] = { AgentRadius, AgentHeight, AgentClimb, MaxSlopeDegrees, CellSize, CellHeight, TileSize, RegionMinArea,
			EdgeMaxError, DetailSampleDistance, DetailSampleMaxError };
		const unsigned char* bytes = reinterpret_cast<const unsigned char*>(values);
		for (size_t i = 0; i < sizeof(values); i++)
		{
			hash = (hash ^ bytes[i]) * 1099511628211ull;
		}
		return hash;
	}

	// Each value within what bakes (a value typed into the Inspector, a scene file edited by hand)
	void Clamp()
	{
		auto clamp = [](float& value, float lo, float hi) { value = value < lo ? lo : value > hi ? hi : value; };
		clamp(AgentRadius, 0.05f, 10.0f);
		clamp(AgentHeight, 0.1f, 20.0f);
		clamp(AgentClimb, 0.0f, 5.0f);
		clamp(MaxSlopeDegrees, 0.0f, 89.0f);
		clamp(CellSize, 0.02f, 2.0f);
		clamp(CellHeight, 0.02f, 2.0f);
		clamp(TileSize, 4.0f, 256.0f);
		clamp(RegionMinArea, 0.0f, 10000.0f);
		clamp(EdgeMaxError, 0.1f, 10.0f);
		clamp(DetailSampleDistance, 0.0f, 32.0f);
		clamp(DetailSampleMaxError, 0.0f, 16.0f);
	}
};
