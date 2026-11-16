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
	// The Nav Areas (Door, Costly) and Nav Links baked in (E1.5), by entity: the polygons of AreaEntities[i] have the area
	// NavAreaIds::FirstArea + i, a link's polygon has its index in LinkEntities as its user id
	std::vector<uint64_t> AreaEntities;
	std::vector<uint64_t> LinkEntities;
};

// The polygons' areas (Detour's 0 to 63; each has its cost in a query's filter) and flags (a query walks only the polygons
// with Walkable): the ground, a Nav Area each (E1.5), and the links by their type
namespace NavAreaIds
{
	constexpr unsigned char Ground = 0;
	constexpr unsigned char FirstArea = 1;     // the first Nav Area's (a Door's or a Costly one's)
	constexpr int MaxAreas = 60;               // Nav Areas with polygons of their own in a navmesh
	constexpr unsigned char LadderLink = 61;   // the links' polygons, by NavLinkComponent::LinkType
	constexpr unsigned char JumpLink = 62;
	constexpr unsigned char WalkLink = 63;
	constexpr unsigned short WalkableFlag = 0x01;

	// The links' costs, times their length: climbing a ladder is slower than walking
	constexpr float LadderCost = 3.0f;
	constexpr float JumpCost = 1.5f;
	constexpr float WalkCost = 1.0f;
}

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

	// A Nav Link as baked (E1.5): its ends, and whether each found the navmesh (one that didn't can't be walked)
	struct LinkInfo
	{
		glm::vec3 Start = glm::vec3(0.0f), End = glm::vec3(0.0f);
		bool Bidirectional = true;
		bool StartJoined = false, EndJoined = false;
		uint8_t Area = 0;     // NavAreaIds' LadderLink, JumpLink or WalkLink
		uint32_t Link = 0;    // its index in NavMeshData::LinkEntities
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

	NavMesh();
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

	// Both made the first time they are asked for and kept with the mesh until its tiles change (the editor's debug view
	// asks each frame); not to be asked for from two threads at once
	const DebugGeometry& GetDebugGeometry() const;
	const Islands& GetIslands() const;
	// A number no other mesh has, and a new one whenever the tiles change: what the debug view's drawing on the GPU is kept by
	uint64_t GetGeometryKey() const { return m_GeometryKey; }
	std::vector<LinkInfo> GetLinks() const;
	// The link a polygon reference is (a crowd agent's link, E1.5): false when it isn't a link's
	bool GetLink(uint64_t polygon, LinkInfo& link) const;

	// ---- The Nav Areas' and Nav Links' states (E1.5), changed while the game runs without a new bake. Each change makes
	// the state version another: a path found before it is to be found again

	// A Nav Area's polygons (area: NavAreaIds::FirstArea on) walkable or closed (a Door locked)
	void SetAreaWalkable(uint8_t area, bool walkable);
	bool IsAreaWalkable(uint8_t area) const { return m_AreaWalkable[area & 63]; }
	// Walking a length in the area costs this times the length (1: as the ground)
	void SetAreaCost(uint8_t area, float cost);
	float GetAreaCost(uint8_t area) const;
	// A Nav Link (its index in NavMeshData::LinkEntities) walkable or not
	void SetLinkEnabled(uint32_t link, bool enabled);
	bool IsLinkEnabled(uint32_t link) const { return link >= m_LinkEnabled.size() || m_LinkEnabled[link]; }
	uint32_t GetStateVersion() const { return m_StateVersion; }
	// The filter the queries walk with: the areas' costs, only the walkable polygons (a crowd copies it)
	const dtQueryFilter* GetFilter() const { return m_Filter; }

	dtNavMesh* GetDetourMesh() const { return m_Mesh; }

private:
	// Each polygon's island (by its tile and index), and the islands' areas
	void FindIslands(std::vector<std::vector<uint32_t>>& islandOfPolygon, std::vector<float>& areas, std::vector<glm::vec2>& heights) const;

	DebugGeometry MakeDebugGeometry() const;
	Islands MakeIslands() const;
	// The tiles changed: a new geometry key, and the kept debug geometry and islands made again when next asked for
	void OnGeometryChanged();

	// The polygon nearest a point, searched in a box around it
	uint64_t FindPolygon(const glm::vec3& position, const glm::vec3& halfExtents, glm::vec3& nearest) const;

	// Each area's polygons and each link's (by its index), found when a tile is added
	void IndexTilePolygons(const void* tile);

	dtNavMesh* m_Mesh = nullptr;
	dtNavMeshQuery* m_Query = nullptr;
	dtQueryFilter* m_Filter = nullptr;
	NavMeshSettings m_Settings;
	std::vector<uint64_t> m_AreaPolygons[64];
	std::vector<std::vector<uint64_t>> m_LinkPolygons;
	bool m_AreaWalkable[64];
	std::vector<bool> m_LinkEnabled;
	uint32_t m_StateVersion = 0;
	uint64_t m_GeometryKey = 0;
	mutable DebugGeometry m_DebugGeometry;
	mutable Islands m_Islands;
	mutable bool m_HasDebugGeometry = false, m_HasIslands = false;
};
