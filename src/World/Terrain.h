#pragma once

#include "Assets/ModelData.h"

#include <glm/glm.hpp>

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

	// The look (shader values: changed at once, nothing is made again). In the terrain's space
	float SandHeight = 4.0f;      // meters above the lowest point: sand below (beaches, river beds), grass above
	float RockSlope = 38.0f;      // degrees: slopes steeper than this are rock
	float RockHeight = 65.0f;     // meters above the lowest point: rock above (bare peaks)

	float Friction = 0.8f;        // physics: what drives and walks on it

	static constexpr float ChunkSize = 64.0f; // meters: the terrain is drawn in squares of this size (culled one by one)
	static constexpr float Spacing = 1.0f;    // meters between two heights
	static constexpr float MinSize = 128.0f, MaxSize = 4096.0f;

	// The settings that change the shape (not the look, not the friction)
	uint64_t GetShapeHash() const;
	// Size a multiple of ChunkSize within the limits, the others within theirs
	void Clamp();
};

// The heights of a terrain: SampleCount x SampleCount of them, a row per z (Heights[z * SampleCount + x]), Spacing meters
// apart, the first at (-Size/2, -Size/2)
struct TerrainHeights
{
	uint32_t SampleCount = 0;
	float Spacing = 1.0f;
	float Size = 0.0f;
	std::vector<float> Heights;
	float MinHeight = 0.0f, MaxHeight = 0.0f;
	uint64_t ShapeHash = 0;  // of the settings they were made from
	float Milliseconds = 0.0f;

	// The height at a point of the terrain's space (between the samples: on the triangles physics has), clamped to its edge
	float GetHeight(float x, float z) const;
	// The normal there (up: +y)
	glm::vec3 GetNormal(float x, float z) const;
	// A ray in the terrain's space: the first point where it meets the ground within maxDistance (t: along direction, which
	// needn't be normalized). False: it misses
	bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, float& t) const;
};

namespace Terrain
{
	// The heights the settings give (on several threads: about 1 s in Debug for 1024 m)
	std::shared_ptr<const TerrainHeights> GenerateHeights(const TerrainSettings& settings);

	// The meshes: one per chunk (ChunkSize meters square), with normals from the heights, tangents along +x, texture
	// coordinates in meters (x, z of the terrain's space: a material's tiling is then repeats per meter) and one material.
	// The triangles of each square split it as physics does (from its corner at -x -z to the one at +x +z)
	ModelData BuildModelData(const TerrainHeights& heights);

	// A random seed for Randomize (any of the 32-bit numbers but 0)
	uint32_t RandomSeed();
}
