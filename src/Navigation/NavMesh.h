#pragma once

#include "Navigation/NavMeshSettings.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

class dtNavMesh;
class dtNavMeshQuery;
class dtQueryFilter;


// One tile as Detour stores it (dtCreateNavMeshData's output): what a bake makes and a file keeps
struct NavMeshTile
{
	int X = 0;
	int Z = 0;
	std::vector<uint8_t> Data;
};

// A baked navmesh as the bake makes it and its file (.mnav, NavMeshFile) keeps it: the tiles and how they lie
struct NavMeshData
{
	NavMeshSettings Settings;            // what it was baked for
	uint64_t SourceHash = 0;             // of what it was baked from (the colliders and the settings): out of date when it differs
	glm::vec3 Origin = glm::vec3(0.0f);  // the baked area's lowest corner: tile (0, 0) starts there
	float TileWorldSize = 0.0f;          // a tile's width in meters (a whole number of cells)
	int TilesX = 0, TilesZ = 0;          // the grid of tiles over the area
	int MaxPolysPerTile = 0;
	int Polygons = 0;
	std::vector<NavMeshTile> Tiles;      // the tiles that have something walkable
};

/**
 * A tiled navmesh and its queries (Detour), for the editor and the player (docs/games/Shooter_Plan.html, E1). Its tiles
 * come from NavMeshBuilder (the editor's bake, Recast) or from a scene's .mnav file (NavMeshFile). Positions are the
 * engine's: meters, Y up.
 */
class NavMesh
{
public:
	using Tile = NavMeshTile;

	// The polygons as triangles in the world, for drawing it (the debug view): the detail mesh's (on the ground's real
	// height), each with its polygon's area and the part (island) it belongs to; and the polygons' edges, each either an
	// outline (nothing walkable beyond it: a wall, a ledge) or an inner edge between two polygons
	struct DebugGeometry
	{
		struct Triangle { glm::vec3 A, B, C; uint8_t Area; uint32_t Island; };
		struct Edge { glm::vec3 A, B; bool Outline; };
		std::vector<Triangle> Triangles;
		std::vector<Edge> Edges;
	};

	// The parts of the mesh one can't walk between (islands, joined only through the polygons' links): their number and,
	// for the largest (by area), its area and the heights it spans. Two floors joined by stairs are one part
	struct Islands
	{
		struct Island
		{
			float Area = 0.0f;              // square meters
			float MinY = 0.0f, MaxY = 0.0f; // the heights its polygons' corners span
		};
		uint32_t Count = 0;
		float TotalArea = 0.0f;
		std::vector<Island> List;           // the largest first
	};

	NavMesh() = default;
	~NavMesh();
	NavMesh(const NavMesh&) = delete;
	NavMesh& operator=(const NavMesh&) = delete;

	// An empty mesh for tiles of tileWorldSize meters from origin (the baked area's lowest corner); false on failure
	bool Create(const glm::vec3& origin, float tileWorldSize, int maxTiles, int maxPolysPerTile, const NavMeshSettings& settings);
	// Adds a tile (copied); false if Detour refuses it
	bool AddTile(const Tile& tile);
	// A baked navmesh's tiles, ready for queries; false (an error logged) if Detour refuses them
	bool Load(const NavMeshData& data);
	void Clear();

	bool IsReady() const { return m_Mesh != nullptr && m_Query != nullptr; }
	int GetTileCount() const;
	int GetPolygonCount() const;
	const NavMeshSettings& GetSettings() const { return m_Settings; }

	// The way from one point to another along the mesh: its corners, the first at the start and the last at the end
	// (or the mesh's nearest point to them). False if either isn't near the mesh, or nothing joins them; a partial path
	// (to the reachable point nearest the end) returns true with *complete false
	bool FindPath(const glm::vec3& from, const glm::vec3& to, std::vector<glm::vec3>& corners, bool* complete = nullptr) const;
	// Whether a straight walk from one point to another stays on the mesh; if not, where it leaves it (*hit)
	bool Raycast(const glm::vec3& from, const glm::vec3& to, glm::vec3* hit = nullptr) const;
	// The mesh's nearest point to a position, within maxDistance across and the agent's height up and down
	bool NearestPoint(const glm::vec3& position, float maxDistance, glm::vec3& point) const;
	// A random point on the mesh within radius of center (across), reachable from center's nearest point (patrols,
	// searching); false if center isn't near the mesh. random: a number in [0, 1) each call (nullptr: the mesh's own)
	bool RandomPointAround(const glm::vec3& center, float radius, glm::vec3& point, float (*random)() = nullptr) const;

	DebugGeometry GetDebugGeometry() const;
	Islands GetIslands() const;

	dtNavMesh* GetDetourMesh() const { return m_Mesh; }

private:
	// Each polygon's island (by its tile and index), and the islands' areas
	void FindIslands(std::vector<std::vector<uint32_t>>& islandOfPolygon, std::vector<float>& areas, std::vector<glm::vec2>& heights) const;

	// The polygon nearest a point, searched in a box around it
	uint64_t FindPolygon(const glm::vec3& position, const glm::vec3& halfExtents, glm::vec3& nearest) const;

	dtNavMesh* m_Mesh = nullptr;
	dtNavMeshQuery* m_Query = nullptr;
	dtQueryFilter* m_Filter = nullptr;
	NavMeshSettings m_Settings;
};
