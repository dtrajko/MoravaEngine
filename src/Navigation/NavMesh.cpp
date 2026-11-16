#include "NavMesh.h"

#include "Core/Log.h"

#include <DetourAlloc.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <cstring>
#include <type_traits>

// The engine builds Detour with 64-bit polygon references (RECASTNAVIGATION_DT_POLYREF64): room for many tiles of many
// polygons, as a large outdoor level needs
static_assert(std::is_same_v<dtPolyRef, uint64_t>, "Detour must be built with DT_POLYREF64");

static constexpr int MaxPathPolygons = 2048;
static constexpr int MaxQueryNodes = 4096;


NavMesh::~NavMesh()
{
	Clear();
}

void NavMesh::Clear()
{
	if (m_Query)
	{
		dtFreeNavMeshQuery(m_Query);
		m_Query = nullptr;
	}
	if (m_Mesh)
	{
		dtFreeNavMesh(m_Mesh);
		m_Mesh = nullptr;
	}
	delete m_Filter;
	m_Filter = nullptr;
}

bool NavMesh::Create(const glm::vec3& origin, float tileWorldSize, int maxTiles, int maxPolysPerTile, const NavMeshSettings& settings)
{
	Clear();
	m_Settings = settings;

	dtNavMeshParams params = {};
	params.orig[0] = origin.x;
	params.orig[1] = origin.y;
	params.orig[2] = origin.z;
	params.tileWidth = tileWorldSize;
	params.tileHeight = tileWorldSize;
	params.maxTiles = maxTiles;
	params.maxPolys = maxPolysPerTile;

	m_Mesh = dtAllocNavMesh();
	if (!m_Mesh || dtStatusFailed(m_Mesh->init(&params)))
	{
		Log::GetLogger()->error("Navigation: the navmesh couldn't be made ({0} tiles of up to {1} polygons)", maxTiles, maxPolysPerTile);
		Clear();
		return false;
	}
	m_Query = dtAllocNavMeshQuery();
	if (!m_Query || dtStatusFailed(m_Query->init(m_Mesh, MaxQueryNodes)))
	{
		Log::GetLogger()->error("Navigation: the navmesh's query couldn't be made");
		Clear();
		return false;
	}
	m_Filter = new dtQueryFilter();
	return true;
}

bool NavMesh::AddTile(const Tile& tile)
{
	if (!m_Mesh || tile.Data.empty())
	{
		return false;
	}
	// Detour keeps the tile's memory and frees it with the mesh (DT_TILE_FREE_DATA): a copy in its own allocation
	unsigned char* data = (unsigned char*)dtAlloc(tile.Data.size(), DT_ALLOC_PERM);
	if (!data)
	{
		return false;
	}
	std::memcpy(data, tile.Data.data(), tile.Data.size());
	if (dtStatusFailed(m_Mesh->addTile(data, (int)tile.Data.size(), DT_TILE_FREE_DATA, 0, nullptr)))
	{
		dtFree(data);
		Log::GetLogger()->error("Navigation: tile ({0}, {1}) couldn't be added to the navmesh", tile.X, tile.Z);
		return false;
	}
	return true;
}

int NavMesh::GetTileCount() const
{
	if (!m_Mesh)
	{
		return 0;
	}
	int count = 0;
	const dtNavMesh* mesh = m_Mesh;
	for (int i = 0; i < mesh->getMaxTiles(); i++)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (tile && tile->header)
		{
			count++;
		}
	}
	return count;
}

int NavMesh::GetPolygonCount() const
{
	if (!m_Mesh)
	{
		return 0;
	}
	int count = 0;
	const dtNavMesh* mesh = m_Mesh;
	for (int i = 0; i < mesh->getMaxTiles(); i++)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (tile && tile->header)
		{
			count += tile->header->polyCount;
		}
	}
	return count;
}

uint64_t NavMesh::FindPolygon(const glm::vec3& position, const glm::vec3& halfExtents, glm::vec3& nearest) const
{
	dtPolyRef polygon = 0;
	if (dtStatusFailed(m_Query->findNearestPoly(&position.x, &halfExtents.x, m_Filter, &polygon, &nearest.x)))
	{
		return 0;
	}
	return polygon;
}

bool NavMesh::FindPath(const glm::vec3& from, const glm::vec3& to, std::vector<glm::vec3>& corners, bool* complete) const
{
	corners.clear();
	if (complete)
	{
		*complete = false;
	}
	if (!IsReady())
	{
		return false;
	}
	// A point counts as on the mesh within an agent's radius or so across, and its height up and down
	const glm::vec3 extents(m_Settings.AgentRadius * 2.0f + m_Settings.CellSize, m_Settings.AgentHeight, m_Settings.AgentRadius * 2.0f + m_Settings.CellSize);
	glm::vec3 start, end;
	const dtPolyRef startPolygon = FindPolygon(from, extents, start);
	const dtPolyRef endPolygon = FindPolygon(to, extents, end);
	if (!startPolygon || !endPolygon)
	{
		return false;
	}

	std::vector<dtPolyRef> path(MaxPathPolygons);
	int pathCount = 0;
	const dtStatus status = m_Query->findPath(startPolygon, endPolygon, &start.x, &end.x, m_Filter, path.data(), &pathCount, MaxPathPolygons);
	if (dtStatusFailed(status) || pathCount == 0)
	{
		return false;
	}
	// A partial path ends at the polygon nearest the goal: its end is the goal's nearest point on that polygon
	glm::vec3 goal = end;
	const bool reached = path[pathCount - 1] == endPolygon;
	if (!reached)
	{
		m_Query->closestPointOnPoly(path[pathCount - 1], &end.x, &goal.x, nullptr);
	}

	std::vector<glm::vec3> straight(MaxPathPolygons);
	int straightCount = 0;
	if (dtStatusFailed(m_Query->findStraightPath(&start.x, &goal.x, path.data(), pathCount, &straight[0].x, nullptr, nullptr,
		&straightCount, MaxPathPolygons)))
	{
		return false;
	}
	corners.assign(straight.begin(), straight.begin() + straightCount);
	if (complete)
	{
		*complete = reached && !dtStatusDetail(status, DT_PARTIAL_RESULT);
	}
	return !corners.empty();
}

bool NavMesh::Raycast(const glm::vec3& from, const glm::vec3& to, glm::vec3* hit) const
{
	if (!IsReady())
	{
		return false;
	}
	const glm::vec3 extents(m_Settings.AgentRadius * 2.0f + m_Settings.CellSize, m_Settings.AgentHeight, m_Settings.AgentRadius * 2.0f + m_Settings.CellSize);
	glm::vec3 start;
	const dtPolyRef startPolygon = FindPolygon(from, extents, start);
	if (!startPolygon)
	{
		if (hit)
		{
			*hit = from;
		}
		return false;
	}
	float t = 0.0f;
	float normal[3];
	dtPolyRef visited[MaxPathPolygons];
	int visitedCount = 0;
	if (dtStatusFailed(m_Query->raycast(startPolygon, &start.x, &to.x, m_Filter, &t, normal, visited, &visitedCount, MaxPathPolygons)))
	{
		return false;
	}
	// t: the share of the way walked before leaving the mesh; FLT_MAX when it never left it
	const bool clear = t > 1.0f;
	if (hit)
	{
		*hit = clear ? to : start + (to - start) * t;
	}
	return clear;
}

bool NavMesh::NearestPoint(const glm::vec3& position, float maxDistance, glm::vec3& point) const
{
	if (!IsReady())
	{
		return false;
	}
	return FindPolygon(position, glm::vec3(maxDistance, m_Settings.AgentHeight, maxDistance), point) != 0;
}
