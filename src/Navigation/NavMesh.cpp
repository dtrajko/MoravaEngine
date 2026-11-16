#include "NavMesh.h"

#include "Core/Log.h"

#include <DetourAlloc.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <cstring>
#include <iterator>
#include <random>
#include <type_traits>

// The engine builds Detour with 64-bit polygon references (RECASTNAVIGATION_DT_POLYREF64): room for many tiles of many
// polygons, as a large outdoor level needs
static_assert(std::is_same_v<dtPolyRef, uint64_t>, "Detour must be built with DT_POLYREF64");

static constexpr int MaxPathPolygons = 2048;
static constexpr int MaxQueryNodes = 4096;

static std::atomic<uint64_t> s_NextGeometryKey{ 1 };


NavMesh::NavMesh()
{
	std::fill(std::begin(m_AreaWalkable), std::end(m_AreaWalkable), true);
	OnGeometryChanged();
}

void NavMesh::OnGeometryChanged()
{
	m_GeometryKey = s_NextGeometryKey++;
	m_DebugGeometry = DebugGeometry();
	m_Islands = Islands();
	m_HasDebugGeometry = m_HasIslands = false;
}

const NavMesh::DebugGeometry& NavMesh::GetDebugGeometry() const
{
	if (!m_HasDebugGeometry)
	{
		m_DebugGeometry = MakeDebugGeometry();
		m_HasDebugGeometry = true;
	}
	return m_DebugGeometry;
}

const NavMesh::Islands& NavMesh::GetIslands() const
{
	if (!m_HasIslands)
	{
		m_Islands = MakeIslands();
		m_HasIslands = true;
	}
	return m_Islands;
}

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
	for (std::vector<uint64_t>& polygons : m_AreaPolygons)
	{
		polygons.clear();
	}
	m_LinkPolygons.clear();
	m_LinkEnabled.clear();
	std::fill(std::begin(m_AreaWalkable), std::end(m_AreaWalkable), true);
	m_StateVersion++;
	OnGeometryChanged();
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
	m_Filter->setIncludeFlags(NavAreaIds::WalkableFlag);
	m_Filter->setAreaCost(NavAreaIds::LadderLink, NavAreaIds::LadderCost);
	m_Filter->setAreaCost(NavAreaIds::JumpLink, NavAreaIds::JumpCost);
	m_Filter->setAreaCost(NavAreaIds::WalkLink, NavAreaIds::WalkCost);
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
	dtTileRef added = 0;
	if (dtStatusFailed(m_Mesh->addTile(data, (int)tile.Data.size(), DT_TILE_FREE_DATA, 0, &added)))
	{
		dtFree(data);
		Log::GetLogger()->error("Navigation: tile ({0}, {1}) couldn't be added to the navmesh", tile.X, tile.Z);
		return false;
	}
	IndexTilePolygons(m_Mesh->getTileByRef(added));
	OnGeometryChanged();
	return true;
}

void NavMesh::IndexTilePolygons(const void* tilePointer)
{
	const dtMeshTile* tile = static_cast<const dtMeshTile*>(tilePointer);
	if (!tile || !tile->header)
	{
		return;
	}
	const dtPolyRef base = m_Mesh->getPolyRefBase(tile);
	for (int p = 0; p < tile->header->polyCount; p++)
	{
		const dtPoly& polygon = tile->polys[p];
		if (polygon.getType() == DT_POLYTYPE_GROUND && polygon.getArea() != NavAreaIds::Ground)
		{
			m_AreaPolygons[polygon.getArea()].push_back(base | (dtPolyRef)p);
		}
	}
	for (int c = 0; c < tile->header->offMeshConCount; c++)
	{
		const dtOffMeshConnection& connection = tile->offMeshCons[c];
		if (connection.userId >= m_LinkPolygons.size())
		{
			m_LinkPolygons.resize(connection.userId + 1);
			m_LinkEnabled.resize(connection.userId + 1, true);
		}
		m_LinkPolygons[connection.userId].push_back(base | (dtPolyRef)connection.poly);
	}
}

void NavMesh::SetAreaWalkable(uint8_t area, bool walkable)
{
	area &= 63;
	if (!m_Mesh || m_AreaWalkable[area] == walkable)
	{
		return;
	}
	m_AreaWalkable[area] = walkable;
	for (dtPolyRef polygon : m_AreaPolygons[area])
	{
		m_Mesh->setPolyFlags(polygon, walkable ? NavAreaIds::WalkableFlag : 0);
	}
	m_StateVersion++;
}

void NavMesh::SetAreaCost(uint8_t area, float cost)
{
	cost = std::max(cost, 0.01f);
	if (!m_Filter || m_Filter->getAreaCost(area & 63) == cost)
	{
		return;
	}
	m_Filter->setAreaCost(area & 63, cost);
	m_StateVersion++;
}

float NavMesh::GetAreaCost(uint8_t area) const
{
	return m_Filter ? m_Filter->getAreaCost(area & 63) : 1.0f;
}

void NavMesh::SetLinkEnabled(uint32_t link, bool enabled)
{
	if (!m_Mesh || link >= m_LinkEnabled.size() || m_LinkEnabled[link] == enabled)
	{
		return;
	}
	m_LinkEnabled[link] = enabled;
	for (dtPolyRef polygon : m_LinkPolygons[link])
	{
		m_Mesh->setPolyFlags(polygon, enabled ? NavAreaIds::WalkableFlag : 0);
	}
	m_StateVersion++;
}

static unsigned int NextPowerOfTwo(unsigned int value)
{
	unsigned int result = 1;
	while (result < value)
	{
		result <<= 1;
	}
	return result;
}

bool NavMesh::Load(const NavMeshData& data)
{
	const int maxTiles = (int)NextPowerOfTwo((unsigned int)std::max(1, data.TilesX * data.TilesZ));
	if (!Create(data.Origin, data.TileWorldSize, maxTiles, data.MaxPolysPerTile, data.Settings))
	{
		return false;
	}
	for (const Tile& tile : data.Tiles)
	{
		if (!AddTile(tile))
		{
			Clear();
			return false;
		}
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
	// t: the share of the way walked before leaving the mesh; FLT_MAX when it never left it. Detour walks across (x and z):
	// the point where it stopped is put on the last polygon's ground
	const bool clear = t > 1.0f;
	if (hit)
	{
		*hit = clear ? to : start + (to - start) * t;
		if (!clear && visitedCount > 0)
		{
			float height = 0.0f;
			if (dtStatusSucceed(m_Query->getPolyHeight(visited[visitedCount - 1], &hit->x, &height)))
			{
				hit->y = height;
			}
		}
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

// The mesh's own random numbers (the main thread's queries): [0, 1)
static float RandomUnit()
{
	static std::minstd_rand generator(12345u);
	return std::uniform_real_distribution<float>(0.0f, 1.0f)(generator) * 0.99999994f;
}

bool NavMesh::RandomPointAround(const glm::vec3& center, float radius, glm::vec3& point, float (*random)()) const
{
	if (!IsReady() || !(radius > 0.0f))
	{
		return false;
	}
	if (!random)
	{
		random = RandomUnit;
	}
	const glm::vec3 extents(m_Settings.AgentRadius * 2.0f + m_Settings.CellSize, m_Settings.AgentHeight, m_Settings.AgentRadius * 2.0f + m_Settings.CellSize);
	glm::vec3 start;
	const dtPolyRef startPolygon = FindPolygon(center, extents, start);
	if (!startPolygon)
	{
		return false;
	}
	// Detour picks a polygon that reaches into the circle (by area) and a point anywhere in it, which can be beyond the
	// radius: a few tries for one inside, else the nearest to the center of those found
	bool found = false;
	float best = FLT_MAX;
	for (int attempt = 0; attempt < 8; attempt++)
	{
		dtPolyRef polygon = 0;
		glm::vec3 candidate;
		if (dtStatusFailed(m_Query->findRandomPointAroundCircle(startPolygon, &start.x, radius, m_Filter, random, &polygon, &candidate.x)) || !polygon)
		{
			continue;
		}
		const float dx = candidate.x - start.x, dz = candidate.z - start.z;
		const float distance = dx * dx + dz * dz;
		if (distance < best)
		{
			best = distance;
			point = candidate;
			found = true;
		}
		if (distance <= radius * radius)
		{
			break;
		}
	}
	return found;
}


// ------------------------------------------------------------------------------------------------- the debug view, islands

// A polygon's corner in the world
static glm::vec3 PolygonVertex(const dtMeshTile* tile, const dtPoly& polygon, int corner)
{
	const float* v = &tile->verts[polygon.verts[corner] * 3];
	return glm::vec3(v[0], v[1], v[2]);
}

void NavMesh::FindIslands(std::vector<std::vector<uint32_t>>& islandOfPolygon, std::vector<float>& areas, std::vector<glm::vec2>& heights) const
{
	const dtNavMesh* mesh = m_Mesh;
	islandOfPolygon.assign(mesh->getMaxTiles(), {});
	for (int i = 0; i < mesh->getMaxTiles(); i++)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (tile && tile->header)
		{
			islandOfPolygon[i].assign(tile->header->polyCount, UINT32_MAX);
		}
	}
	// A flood over the polygons' links (within a tile, across tiles, and the links of E1.5)
	std::vector<std::pair<int, int>> stack;
	for (int i = 0; i < mesh->getMaxTiles(); i++)
	{
		for (int p = 0; p < (int)islandOfPolygon[i].size(); p++)
		{
			if (islandOfPolygon[i][p] != UINT32_MAX)
			{
				continue;
			}
			const uint32_t island = (uint32_t)areas.size();
			areas.push_back(0.0f);
			heights.push_back(glm::vec2(FLT_MAX, -FLT_MAX));
			islandOfPolygon[i][p] = island;
			stack.push_back({ i, p });
			while (!stack.empty())
			{
				const auto [ti, pi] = stack.back();
				stack.pop_back();
				const dtMeshTile* tile = mesh->getTile(ti);
				const dtPoly& polygon = tile->polys[pi];
				if (polygon.getType() == DT_POLYTYPE_GROUND)
				{
					const glm::vec3 first = PolygonVertex(tile, polygon, 0);
					for (int c = 0; c < polygon.vertCount; c++)
					{
						const glm::vec3 v = PolygonVertex(tile, polygon, c);
						heights[island].x = std::min(heights[island].x, v.y);
						heights[island].y = std::max(heights[island].y, v.y);
						if (c >= 2)
						{
							areas[island] += 0.5f * glm::length(glm::cross(PolygonVertex(tile, polygon, c - 1) - first, v - first));
						}
					}
				}
				for (unsigned int k = polygon.firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
				{
					unsigned int salt = 0, it = 0, ip = 0;
					mesh->decodePolyId(tile->links[k].ref, salt, it, ip);
					if (it < islandOfPolygon.size() && ip < islandOfPolygon[it].size() && islandOfPolygon[it][ip] == UINT32_MAX)
					{
						islandOfPolygon[it][ip] = island;
						stack.push_back({ (int)it, (int)ip });
					}
				}
			}
		}
	}
}

NavMesh::Islands NavMesh::MakeIslands() const
{
	Islands result;
	if (!m_Mesh)
	{
		return result;
	}
	std::vector<std::vector<uint32_t>> islandOfPolygon;
	std::vector<float> areas;
	std::vector<glm::vec2> heights;
	FindIslands(islandOfPolygon, areas, heights);
	result.Count = (uint32_t)areas.size();
	for (size_t i = 0; i < areas.size(); i++)
	{
		result.TotalArea += areas[i];
		result.List.push_back({ areas[i], heights[i].x, heights[i].y });
	}
	std::sort(result.List.begin(), result.List.end(), [](const Islands::Island& a, const Islands::Island& b) { return a.Area > b.Area; });
	return result;
}

// A link's ends from its connection, and whether each is joined to a polygon (Detour's links of its polygon: edge 0 the
// start's, 1 the end's)
static NavMesh::LinkInfo MakeLinkInfo(const dtMeshTile* tile, const dtOffMeshConnection& connection)
{
	NavMesh::LinkInfo info;
	info.Start = glm::vec3(connection.pos[0], connection.pos[1], connection.pos[2]);
	info.End = glm::vec3(connection.pos[3], connection.pos[4], connection.pos[5]);
	info.Bidirectional = (connection.flags & DT_OFFMESH_CON_BIDIR) != 0;
	info.Link = connection.userId;
	const dtPoly& polygon = tile->polys[connection.poly];
	info.Area = polygon.getArea();
	for (unsigned int k = polygon.firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
	{
		(tile->links[k].edge == 0 ? info.StartJoined : info.EndJoined) = true;
	}
	return info;
}

std::vector<NavMesh::LinkInfo> NavMesh::GetLinks() const
{
	std::vector<LinkInfo> links;
	if (!m_Mesh)
	{
		return links;
	}
	const dtNavMesh* mesh = m_Mesh;
	for (int i = 0; i < mesh->getMaxTiles(); i++)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header)
		{
			continue;
		}
		for (int c = 0; c < tile->header->offMeshConCount; c++)
		{
			links.push_back(MakeLinkInfo(tile, tile->offMeshCons[c]));
		}
	}
	return links;
}

bool NavMesh::GetLink(uint64_t polygon, LinkInfo& link) const
{
	const dtMeshTile* tile = nullptr;
	const dtPoly* poly = nullptr;
	if (!m_Mesh || dtStatusFailed(m_Mesh->getTileAndPolyByRef(polygon, &tile, &poly)) || poly->getType() != DT_POLYTYPE_OFFMESH_CONNECTION)
	{
		return false;
	}
	const unsigned int index = (unsigned int)(poly - tile->polys);
	for (int c = 0; c < tile->header->offMeshConCount; c++)
	{
		if (tile->offMeshCons[c].poly == index)
		{
			link = MakeLinkInfo(tile, tile->offMeshCons[c]);
			return true;
		}
	}
	return false;
}

NavMesh::DebugGeometry NavMesh::MakeDebugGeometry() const
{
	DebugGeometry geometry;
	if (!m_Mesh)
	{
		return geometry;
	}
	std::vector<std::vector<uint32_t>> islandOfPolygon;
	std::vector<float> areas;
	std::vector<glm::vec2> heights;
	FindIslands(islandOfPolygon, areas, heights);
	// The islands by size, the largest 0: its color stays when a small island comes or goes
	std::vector<uint32_t> order(areas.size()), rank(areas.size());
	for (uint32_t i = 0; i < (uint32_t)order.size(); i++)
	{
		order[i] = i;
	}
	std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return areas[a] > areas[b]; });
	for (uint32_t i = 0; i < (uint32_t)order.size(); i++)
	{
		rank[order[i]] = i;
	}

	const dtNavMesh* mesh = m_Mesh;
	for (int i = 0; i < mesh->getMaxTiles(); i++)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header)
		{
			continue;
		}
		for (int p = 0; p < tile->header->polyCount; p++)
		{
			const dtPoly& polygon = tile->polys[p];
			if (polygon.getType() != DT_POLYTYPE_GROUND)
			{
				continue; // the links (E1.5) are drawn as arcs
			}
			const uint32_t island = rank[islandOfPolygon[i][p]];
			// The detail mesh's triangles: its corners are the polygon's (index < vertCount) or the detail's own
			const dtPolyDetail& detail = tile->detailMeshes[p];
			for (int t = 0; t < detail.triCount; t++)
			{
				const unsigned char* corners = &tile->detailTris[(detail.triBase + t) * 4];
				glm::vec3 v[3];
				for (int k = 0; k < 3; k++)
				{
					const float* source = corners[k] < polygon.vertCount ? &tile->verts[polygon.verts[corners[k]] * 3]
						: &tile->detailVerts[(detail.vertBase + corners[k] - polygon.vertCount) * 3];
					v[k] = glm::vec3(source[0], source[1], source[2]);
				}
				geometry.Triangles.push_back({ v[0], v[1], v[2], polygon.getArea(), island });
			}
			// The edges: an outline where no polygon is beyond (none inside the tile, and no link across its border)
			for (int e = 0; e < polygon.vertCount; e++)
			{
				bool outline = polygon.neis[e] == 0;
				if (polygon.neis[e] & DT_EXT_LINK)
				{
					outline = true;
					for (unsigned int k = polygon.firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
					{
						if (tile->links[k].edge == e)
						{
							outline = false;
							break;
						}
					}
				}
				else if (!outline && (int)(polygon.neis[e] - 1) < p)
				{
					continue; // an inner edge once, from the polygon with the lower index
				}
				geometry.Edges.push_back({ PolygonVertex(tile, polygon, e), PolygonVertex(tile, polygon, (e + 1) % polygon.vertCount), outline });
			}
		}
	}
	return geometry;
}
