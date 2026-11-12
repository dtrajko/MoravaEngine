#pragma once

#include "Assets/ModelData.h"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>


/**
 * A terrain made from a seed (docs/engine/Game_Engine_Runtime_Guide.html, the terrain): its settings, the heights they give,
 * and the meshes and the shape for physics made from the heights. Plain data and code, without a graphics API: the editor,
 * the player, physics and the renderer all use it.
 *
 * The same settings give the same terrain on every machine and every platform: the noise is the engine's own (a seeded
 * permutation of its own random numbers, not the standard library's, whose shuffle differs between compilers), so a seed
 * saved in a scene is the terrain the game was made on.
 *
 * Space: the terrain's own, the entity's. A square of Size meters around the entity's position, with a height every
 * Spacing meters: x and z from -Size/2 to Size/2, y from 0 (the lowest the noise reaches) up to about Height.
 */
struct TerrainSettings
{
	// The shape (a change of any of these makes the terrain again: GetShapeHash)
	uint32_t Seed = 1;
	float Size = 1024.0f;         // meters, a multiple of ChunkSize
	float Height = 80.0f;         // meters from the lowest valley to the highest peak, about
	float FeatureSize = 450.0f;   // meters: the size of the largest hills
	uint32_t Octaves = 6;         // layers of ever smaller detail on the hills (more of it high up than in the valleys)
	float Roughness = 0.45f;      // how strong each smaller layer is (0.3 smooth, 0.6 rugged)
	float Ridges = 0.3f;          // 0: round hills; 1: sharp ridges (mountains)
	float Valleys = 1.5f;         // above 1: wide flat lowlands and steep peaks; 1: the noise as it is
	float RimHeight = 60.0f;      // meters above Height the ground rises along the edges, about (0: no rim): hills around the
	                              // terrain, so a camera near the ground sees them against the sky, never the sky map's ground
	float RimWidth = 220.0f;      // meters from the edge over which it rises
	glm::vec2 FlatCenter = glm::vec2(0.0f); // where the flat area is (a start for the player), in the terrain's space
	float FlatRadius = 30.0f;     // meters of level ground there (0: none)
	float FlatBlend = 60.0f;      // meters over which it rises back into the hills
	// Guides (TerrainGuides: a road): near them the ground follows them rather than its own hills. Just below a guide and
	// beside it (an embankment), then hills at about its height (some higher, some lower: where they dip below it, the
	// guide stands on its supports), and beyond Guide Band the terrain's own hills again. Without guides, these do nothing
	float GuideClearance = 0.5f;  // meters the ground stays below a guide's surface, under it and beside it
	float GuideShoulder = 6.0f;   // meters beside a guide where the ground stays at its height (before the hills rise)
	float GuideRise = 40.0f;      // meters over which the hills beside it rise to their full height
	float GuideHills = 12.0f;     // meters the hills near a guide go above or below its height, at most
	float GuideHillSize = 150.0f; // meters: the size of those hills
	float GuideBand = 150.0f;     // meters from a guide the ground keeps to its height
	float GuideBlend = 200.0f;    // meters over which it turns into the terrain's own hills beyond the band

	float Spacing = 1.0f;         // meters between two heights: 1, 2 or 4. Coarser is a quarter of the triangles (and of the
	                              // heights physics has) per step, with the same hills: the small bumps go
	uint32_t MaxVertices = 0;     // the drawn mesh's vertices at the most (0: as many as its exact shape needs). Drawing only:
	                              // physics has every height. The chunks that stray furthest from the heights get them first

	// The look (shader values: changed at once, nothing is made again). In the terrain's space
	float SandHeight = 4.0f;      // meters above the lowest point: sand below (beaches, river beds), grass above
	float RockSlope = 38.0f;      // degrees: slopes steeper than this are rock
	float RockHeight = 65.0f;     // meters above the lowest point: rock above (bare peaks)

	float Friction = 0.8f;        // physics: what drives and walks on it

	// The detail (drawing only: physics always has every height): each chunk is drawn with as few of its heights as keep
	// its shape within this many meters of the true one per 100 m of distance from the camera (0.5: half a meter at 100 m,
	// two meters at 400 m). Flat ground is a few triangles at any distance; larger is fewer triangles
	float LodTolerance = 1.0f;

	static constexpr float ChunkSize = 64.0f; // meters: Size is a multiple of it
	// The terrain is drawn in squares of this many cells (culled one by one, each a draw call per pass): 64 m at 1 m
	// spacing, 256 m at 4 m. Coarser spacing, larger squares: as many heights each, far fewer draw calls
	static constexpr uint32_t ChunkCells = 64;
	static constexpr float MinSize = 128.0f, MaxSize = 4096.0f;

	// The settings that change the shape (not the look, not the friction)
	uint64_t GetShapeHash() const;
	// Size a multiple of ChunkSize within the limits, the others within theirs
	void Clamp();
};

// What a terrain follows (TerrainSettings' Guide values): the surfaces of its guides (a road's driven surface), as triangles
// in the terrain's space. The renderer, physics and the editor gather them from the guide entities the same way
// (EnvMapVulkanTerrain::GatherGuides), so they all have the same heights
struct TerrainGuides
{
	std::vector<std::array<glm::vec3, 3>> Triangles;
	uint64_t Hash = 0; // what they were gathered from (0: no guides): part of the heights' ShapeHash
};

// The heights of a terrain: SampleCount x SampleCount of them, a row per z (Heights[z * SampleCount + x]), Spacing meters
// apart, the first at (-Size/2, -Size/2)
struct TerrainHeights
{
	uint32_t SampleCount = 0;
	float Spacing = 1.0f;
	float Size = 0.0f;
	std::vector<float> Heights;
	// Under and beside the guides (TerrainGuides): the highest the drawn ground may be at each height (as Heights; empty:
	// no guides), a little below their surface. A level of detail whose triangles would rise above it there is never drawn
	// (BuildModelData), so the ground never shows through a road, however far away
	std::vector<float> Ceiling;
	float MinHeight = 0.0f, MaxHeight = 0.0f;
	uint64_t ShapeHash = 0;  // of the settings and the guides they were made from (Terrain::GetShapeHash)
	float Milliseconds = 0.0f;

	// The height at a point of the terrain's space (between the samples: on the triangles physics has), clamped to its edge
	float GetHeight(float x, float z) const;
	// The normal there (up: +y)
	glm::vec3 GetNormal(float x, float z) const;
	// A ray in the terrain's space: the first point where it meets the ground within maxDistance (t: along direction, which
	// needn't be normalized). False: it misses
	bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, float& t) const;
};

// The chunks' levels of detail (BuildModelData): each chunk's triangles with every height (level 0) and with every 2nd,
// 4th... 64th height (levels 1 to 6; the last that fits a chunk is it as two triangles), each level's error (how far its triangles stray from the heights), and for each level a
// skirt: a strip hanging down from the chunk's edges (both sides facing out), which hides the gaps where two chunks of
// different levels meet. A level's triangles and its skirt are drawn together.
// A chunk's own mesh (its vertices and triangles) is its coarsest exact level, the base: flat or evenly sloped ground is
// four vertices and two triangles. Levels finer than the base are the base; the coarser ones follow the meshes' own
// triangles in the model's index buffer
struct TerrainLods
{
	static constexpr uint32_t LevelCount = 7;
	struct Range
	{
		uint32_t BaseIndex = 0;  // the first index (as a mesh's BaseIndex: indices, not triangles)
		uint32_t IndexCount = 0;
	};
	struct Chunk
	{
		Range Surface[LevelCount]; // the base level's (and every finer one's): the mesh's own range
		Range Skirt[LevelCount];
		float Error[LevelCount] = {}; // meters: the most a level's triangles are above or below a height (level 0: none);
		                              // infinite: above the ceiling under a guide, never drawn
	};
	std::vector<Chunk> Chunks; // as the model's meshes

	// The coarsest level of a chunk whose error is within the tolerance (TerrainSettings::LodTolerance) at this distance
	// from the camera (meters, to the nearest point of its box)
	static uint32_t LevelAt(const Chunk& chunk, float distance, float tolerance);
};

namespace Terrain
{
	// What the heights depend on: the shape's settings and the guides (guidesHash: TerrainGuides::Hash, 0: none)
	uint64_t GetShapeHash(const TerrainSettings& settings, uint64_t guidesHash);

	// The heights the settings give (on several threads: about 1 s in Debug for 1024 m), following the guides (optional)
	std::shared_ptr<const TerrainHeights> GenerateHeights(const TerrainSettings& settings, const TerrainGuides* guides = nullptr);

	// The meshes: one per chunk (ChunkCells heights square), with normals from the heights, tangents along +x, texture
	// coordinates in meters (x, z of the terrain's space: a material's tiling is then repeats per meter) and one material.
	// The triangles of each square split it as physics does (from its corner at -x -z to the one at +x +z)
	// lods (optional): the chunks' levels of detail, written into the model's indices after the meshes' own (see TerrainLods)
	// maxVertices: TerrainSettings::MaxVertices (0: no limit)
	ModelData BuildModelData(const TerrainHeights& heights, TerrainLods* lods = nullptr, uint32_t maxVertices = 0);

	// A random seed for Randomize (any of the 32-bit numbers but 0)
	uint32_t RandomSeed();
}
