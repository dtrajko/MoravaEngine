#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>


struct TerrainHeights;

/**
 * Trees, bushes and rocks scattered over a terrain (the racing game's Phase 8: docs/games/Racing_Plan.html; the engine guide's
 * foliage section): the settings, and where the scatter puts each copy. Plain data and code without a graphics API, like the
 * terrain (World/Terrain.h): the editor, the player and the renderer use it, and the same settings give the same places on
 * every machine (the engine's own random numbers, not the standard library's).
 *
 * A species is one model and the rules for where its copies stand: how many per hectare, on how steep a slope and between
 * which heights, how big, and how far from everything else that has a collider (the keep-out: a road, a wall, a house). The
 * scatter is even, not clumped: the terrain is divided into squares one copy's share of the ground wide, and each square gets
 * a copy at a random place in it, kept or left out by the rules. With Max Distance a species grows only near the colliders
 * (along a track: none on the far sides of the hills, which nobody sees). The copies are grouped by cells (CellSize meters square),
 * which the renderer culls and draws one by one.
 *
 * Space: the terrain's (its entity's), as TerrainHeights.
 */
struct FoliageSpecies
{
	std::string Model;          // the model file (absolute here; in the scene file relative to the project, with its asset ID)
	float Density = 20.0f;      // copies per hectare (100 m x 100 m) where the rules let them stand
	float MinScale = 0.8f;      // each copy's size, times the model's: random between these
	float MaxScale = 1.2f;
	float MaxSlope = 30.0f;     // degrees: none on steeper ground
	float MinHeight = -1.0e6f;  // meters, the terrain's y: none below or above (sand, bare peaks)
	float MaxHeight = 1.0e6f;
	float KeepOut = 4.0f;       // meters: none this close to a collider other than the terrain (a road, its walls), from
	                            // the copy's edge (its footprint: its crown, not its trunk), so none reaches over a road
	float MaxDistance = 0.0f;   // meters: only this close to one (along a road: what its driver sees). 0: anywhere
	float AlignToSlope = 0.0f;  // 0: upright (trees); 1: tilted with the ground (bushes, rocks); between: part of the way
	float Sink = 0.2f;          // meters below the ground: no gap under a copy on a slope

	static constexpr float MaxDensity = 2000.0f; // per hectare: a copy every 2.2 m
};

struct FoliageSettings
{
	uint32_t Seed = 1;
	std::vector<FoliageSpecies> Species;
	float CellSize = 64.0f;       // meters: the squares the copies are grouped in (culled and drawn one by one)
	float DrawDistance = 600.0f;  // meters from the camera: cells farther away aren't drawn
	float ShadowDistance = 250.0f; // meters from the camera: cells farther away cast no shadow
	float ClearHeight = 100.0f;   // meters above the ground the keep-out looks for colliders (a bridge or a road overhead)

	static constexpr uint32_t MaxCopies = 500000;

	// The settings that change where the copies are (not the distances)
	uint64_t GetScatterHash() const;
	void Clamp();
};

// Where the scatter put the copies: each copy's species and transform (the terrain's space), grouped by cell
struct FoliageInstances
{
	struct Copy
	{
		glm::mat4 Transform = glm::mat4(1.0f);
		uint32_t Species = 0;
	};
	struct Cell
	{
		glm::vec2 Min = glm::vec2(0.0f), Max = glm::vec2(0.0f); // x, z in the terrain's space
		uint32_t First = 0;  // its copies: Copies[First, First + Count), sorted by species
		uint32_t Count = 0;
	};
	std::vector<Copy> Copies;
	std::vector<Cell> Cells;               // only cells with copies
	std::vector<uint32_t> CopiesBySpecies; // how many each species has
	uint32_t KeptOut = 0;                  // places the keep-out took (for the Inspector's summary)
	uint64_t ScatterHash = 0;              // of the settings and the terrain they were made from
	float Milliseconds = 0.0f;
};

namespace Foliage
{
	// True: something other than the terrain is within radius of the point (on the ground, the terrain's space) or above it
	// up to height meters. Asked with a species' Keep Out (true: no copy there) and its Max Distance (false: no copy there)
	using KeepOutQuery = std::function<bool(const glm::vec3& ground, float radius, float height)>;

	// What the scatter's result depends on: the settings that move copies and the terrain's shape (FoliageInstances::ScatterHash)
	uint64_t GetScatterHash(const FoliageSettings& settings, const TerrainHeights& terrain);

	// The copies of every species over the terrain. keepOut: may be empty (no keep-out). footprints (optional, by species):
	// meters from a copy's place to the farthest its model reaches sideways at scale 1, whichever way it is turned; Keep Out
	// counts from there
	std::shared_ptr<const FoliageInstances> Scatter(const FoliageSettings& settings, const TerrainHeights& terrain, const KeepOutQuery& keepOut,
		const std::vector<float>* footprints = nullptr);
}
