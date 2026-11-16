#pragma once

#include "Navigation/NavMeshSettings.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

class dtNavMesh;
class dtNavMeshQuery;
class dtQueryFilter;


/**
 * A tiled navmesh and its queries (Detour), for the editor and the player (docs/games/Shooter_Plan.html, E1). Its tiles
 * come from NavMeshBuilder (the editor's bake, Recast) or, later, from a scene's .mnav file. Positions are the engine's:
 * meters, Y up.
 */
class NavMesh
{
public:
	// One tile as Detour stores it (dtCreateNavMeshData's output): what a bake makes and a file keeps
	struct Tile
	{
		int X = 0;
		int Z = 0;
		std::vector<uint8_t> Data;
	};

	NavMesh() = default;
	~NavMesh();
	NavMesh(const NavMesh&) = delete;
	NavMesh& operator=(const NavMesh&) = delete;

	// An empty mesh for tiles of tileWorldSize meters from origin (the baked area's lowest corner); false on failure
	bool Create(const glm::vec3& origin, float tileWorldSize, int maxTiles, int maxPolysPerTile, const NavMeshSettings& settings);
	// Adds a tile (copied); false if Detour refuses it
	bool AddTile(const Tile& tile);
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

	dtNavMesh* GetDetourMesh() const { return m_Mesh; }

private:
	// The polygon nearest a point, searched in a box around it
	uint64_t FindPolygon(const glm::vec3& position, const glm::vec3& halfExtents, glm::vec3& nearest) const;

	dtNavMesh* m_Mesh = nullptr;
	dtNavMeshQuery* m_Query = nullptr;
	dtQueryFilter* m_Filter = nullptr;
	NavMeshSettings m_Settings;
};
