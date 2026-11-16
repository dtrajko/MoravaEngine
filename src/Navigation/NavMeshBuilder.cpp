#include "NavMeshBuilder.h"

#include "Core/Log.h"

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <Recast.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>


namespace NavMeshBuilder
{
	// The polygons' area and flags: every baked polygon is ground one can walk (Nav Areas, E1.5, add others)
	static constexpr unsigned char GroundArea = 0;
	static constexpr unsigned short WalkFlag = 0x01;

	// Recast's structures, freed when leaving the tile's bake whatever happens
	struct RecastTile
	{
		rcHeightfield* Heightfield = nullptr;
		rcCompactHeightfield* Compact = nullptr;
		rcContourSet* Contours = nullptr;
		rcPolyMesh* PolyMesh = nullptr;
		rcPolyMeshDetail* Detail = nullptr;

		~RecastTile()
		{
			rcFreeHeightField(Heightfield);
			rcFreeCompactHeightfield(Compact);
			rcFreeContourSet(Contours);
			rcFreePolyMesh(PolyMesh);
			rcFreePolyMeshDetail(Detail);
		}
	};

	// One tile: the triangles over it rasterized into cells, the walkable cells found, outlined, and cut into polygons.
	// Returns false on an error; an empty tile (nothing walkable) returns true with no data
	static bool BuildTile(rcContext& context, const rcConfig& base, const Input& input, const std::vector<int>& triangles,
		const std::vector<float>& vertices, int tileX, int tileZ, const float* tileMin, const float* tileMax,
		NavMeshTile& tile, std::string& error)
	{
		rcConfig config = base;
		rcVcopy(config.bmin, tileMin);
		rcVcopy(config.bmax, tileMax);
		// The tile's own area and a border around it, so the tiles' edges meet: what the agent's radius erodes at the
		// tile's edge comes from its neighbors' geometry
		config.bmin[0] -= config.borderSize * config.cs;
		config.bmin[2] -= config.borderSize * config.cs;
		config.bmax[0] += config.borderSize * config.cs;
		config.bmax[2] += config.borderSize * config.cs;

		RecastTile t;
		t.Heightfield = rcAllocHeightfield();
		if (!t.Heightfield || !rcCreateHeightfield(&context, *t.Heightfield, config.width, config.height, config.bmin, config.bmax,
			config.cs, config.ch))
		{
			error = "out of memory for a tile's heightfield";
			return false;
		}

		// The triangles over the tile (by their bounding boxes), walkable where they aren't too steep
		std::vector<int> indices;
		indices.reserve(triangles.size() * 3);
		for (int triangle : triangles)
		{
			const uint32_t* index = &input.Indices[triangle * 3];
			float lo[2] = { 1e30f, 1e30f }, hi[2] = { -1e30f, -1e30f };
			for (int k = 0; k < 3; k++)
			{
				const glm::vec3& v = input.Vertices[index[k]];
				lo[0] = std::min(lo[0], v.x); hi[0] = std::max(hi[0], v.x);
				lo[1] = std::min(lo[1], v.z); hi[1] = std::max(hi[1], v.z);
			}
			if (hi[0] < config.bmin[0] || lo[0] > config.bmax[0] || hi[1] < config.bmin[2] || lo[1] > config.bmax[2])
			{
				continue;
			}
			indices.push_back((int)index[0]);
			indices.push_back((int)index[1]);
			indices.push_back((int)index[2]);
		}
		const int triangleCount = (int)indices.size() / 3;
		if (triangleCount == 0)
		{
			return true;
		}
		std::vector<unsigned char> areas(triangleCount, 0);
		rcMarkWalkableTriangles(&context, config.walkableSlopeAngle, vertices.data(), (int)input.Vertices.size(), indices.data(),
			triangleCount, areas.data());
		if (!rcRasterizeTriangles(&context, vertices.data(), (int)input.Vertices.size(), indices.data(), areas.data(), triangleCount,
			*t.Heightfield, config.walkableClimb))
		{
			error = "the triangles couldn't be rasterized";
			return false;
		}

		// Walkable only where one can step up onto, isn't at a ledge's edge, and has the agent's height above
		rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *t.Heightfield);
		rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *t.Heightfield);
		rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *t.Heightfield);

		t.Compact = rcAllocCompactHeightfield();
		if (!t.Compact || !rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb, *t.Heightfield, *t.Compact))
		{
			error = "the compact heightfield couldn't be built";
			return false;
		}
		rcFreeHeightField(t.Heightfield);
		t.Heightfield = nullptr;
		// The agent's radius away from walls and edges
		if (!rcErodeWalkableArea(&context, config.walkableRadius, *t.Compact))
		{
			error = "the walkable area couldn't be eroded";
			return false;
		}
		// Regions by the distance field (watershed): the best polygons for open floors and rooms
		if (!rcBuildDistanceField(&context, *t.Compact) ||
			!rcBuildRegions(&context, *t.Compact, config.borderSize, config.minRegionArea, config.mergeRegionArea))
		{
			error = "the regions couldn't be built";
			return false;
		}

		t.Contours = rcAllocContourSet();
		if (!t.Contours || !rcBuildContours(&context, *t.Compact, config.maxSimplificationError, config.maxEdgeLen, *t.Contours))
		{
			error = "the contours couldn't be built";
			return false;
		}
		if (t.Contours->nconts == 0)
		{
			return true;
		}
		t.PolyMesh = rcAllocPolyMesh();
		if (!t.PolyMesh || !rcBuildPolyMesh(&context, *t.Contours, config.maxVertsPerPoly, *t.PolyMesh))
		{
			error = "the polygons couldn't be built";
			return false;
		}
		t.Detail = rcAllocPolyMeshDetail();
		if (!t.Detail || !rcBuildPolyMeshDetail(&context, *t.PolyMesh, *t.Compact, config.detailSampleDist,
			config.detailSampleMaxError, *t.Detail))
		{
			error = "the detail mesh couldn't be built";
			return false;
		}
		if (t.PolyMesh->npolys == 0)
		{
			return true;
		}
		if (t.PolyMesh->nverts >= 0xffff)
		{
			error = "a tile has too many vertices (a smaller TileSize helps)";
			return false;
		}
		for (int i = 0; i < t.PolyMesh->npolys; i++)
		{
			if (t.PolyMesh->areas[i] == RC_WALKABLE_AREA)
			{
				t.PolyMesh->areas[i] = GroundArea;
			}
			t.PolyMesh->flags[i] = WalkFlag;
		}

		dtNavMeshCreateParams params = {};
		params.verts = t.PolyMesh->verts;
		params.vertCount = t.PolyMesh->nverts;
		params.polys = t.PolyMesh->polys;
		params.polyAreas = t.PolyMesh->areas;
		params.polyFlags = t.PolyMesh->flags;
		params.polyCount = t.PolyMesh->npolys;
		params.nvp = t.PolyMesh->nvp;
		params.detailMeshes = t.Detail->meshes;
		params.detailVerts = t.Detail->verts;
		params.detailVertsCount = t.Detail->nverts;
		params.detailTris = t.Detail->tris;
		params.detailTriCount = t.Detail->ntris;
		params.walkableHeight = base.walkableHeight * base.ch;
		params.walkableRadius = base.walkableRadius * base.cs;
		params.walkableClimb = base.walkableClimb * base.ch;
		params.tileX = tileX;
		params.tileY = tileZ;
		params.tileLayer = 0;
		rcVcopy(params.bmin, t.PolyMesh->bmin);
		rcVcopy(params.bmax, t.PolyMesh->bmax);
		params.cs = config.cs;
		params.ch = config.ch;
		params.buildBvTree = true;

		unsigned char* data = nullptr;
		int size = 0;
		if (!dtCreateNavMeshData(&params, &data, &size))
		{
			error = "Detour couldn't make a tile's data";
			return false;
		}
		tile.X = tileX;
		tile.Z = tileZ;
		tile.Data.assign(data, data + size);
		dtFree(data);
		return true;
	}

	Result Build(const Input& input, const NavMeshSettings& settings, Progress* progress)
	{
		Result result;
		NavMeshData& data = result.Data;
		data.Settings = settings;
		const auto start = std::chrono::steady_clock::now();
		if (input.Vertices.empty() || input.Indices.size() < 3)
		{
			result.Error = "nothing to bake (no static colliders)";
			return result;
		}

		std::vector<float> vertices(input.Vertices.size() * 3);
		for (size_t i = 0; i < input.Vertices.size(); i++)
		{
			vertices[i * 3 + 0] = input.Vertices[i].x;
			vertices[i * 3 + 1] = input.Vertices[i].y;
			vertices[i * 3 + 2] = input.Vertices[i].z;
		}
		float boundsMin[3], boundsMax[3];
		rcCalcBounds(vertices.data(), (int)input.Vertices.size(), boundsMin, boundsMax);

		rcConfig config = {};
		config.cs = settings.CellSize;
		config.ch = settings.CellHeight;
		config.walkableSlopeAngle = settings.MaxSlopeDegrees;
		config.walkableHeight = (int)std::ceil(settings.AgentHeight / config.ch);
		config.walkableClimb = (int)std::floor(settings.AgentClimb / config.ch);
		config.walkableRadius = (int)std::ceil(settings.AgentRadius / config.cs);
		config.maxEdgeLen = (int)(12.0f / config.cs);
		config.maxSimplificationError = settings.EdgeMaxError;
		config.minRegionArea = (int)(settings.RegionMinArea / (config.cs * config.cs));
		config.mergeRegionArea = (int)(20.0f / (config.cs * config.cs));
		config.maxVertsPerPoly = DT_VERTS_PER_POLYGON;
		config.tileSize = std::max(16, (int)(settings.TileSize / config.cs));
		config.borderSize = config.walkableRadius + 3;
		config.width = config.tileSize + config.borderSize * 2;
		config.height = config.tileSize + config.borderSize * 2;
		config.detailSampleDist = settings.DetailSampleDistance < 0.9f ? 0.0f : config.cs * settings.DetailSampleDistance;
		config.detailSampleMaxError = config.ch * settings.DetailSampleMaxError;

		int gridWidth = 0, gridHeight = 0;
		rcCalcGridSize(boundsMin, boundsMax, config.cs, &gridWidth, &gridHeight);
		data.TileWorldSize = config.tileSize * config.cs;
		data.TilesX = (gridWidth + config.tileSize - 1) / config.tileSize;
		data.TilesZ = (gridHeight + config.tileSize - 1) / config.tileSize;
		data.Origin = glm::vec3(boundsMin[0], boundsMin[1], boundsMin[2]);
		data.MaxPolysPerTile = 1 << 14;
		if ((int64_t)data.TilesX * data.TilesZ > (1 << 20))
		{
			result.Error = "the area is too large for its tiles (" + std::to_string(data.TilesX) + " x " + std::to_string(data.TilesZ) +
				"): a larger Tile Size helps";
			return result;
		}
		if (progress)
		{
			progress->TilesTotal = data.TilesX * data.TilesZ;
			progress->TilesDone = 0;
		}

		// Each triangle in the tiles its bounding box (with a tile's border) touches: a tile looks only at its own
		const float border = config.borderSize * config.cs;
		std::vector<std::vector<int>> tileTriangles((size_t)data.TilesX * data.TilesZ);
		for (int triangle = 0; triangle < (int)(input.Indices.size() / 3); triangle++)
		{
			float lo[2] = { 1e30f, 1e30f }, hi[2] = { -1e30f, -1e30f };
			for (int k = 0; k < 3; k++)
			{
				const glm::vec3& v = input.Vertices[input.Indices[triangle * 3 + k]];
				lo[0] = std::min(lo[0], v.x); hi[0] = std::max(hi[0], v.x);
				lo[1] = std::min(lo[1], v.z); hi[1] = std::max(hi[1], v.z);
			}
			const int x0 = std::max(0, (int)std::floor((lo[0] - border - boundsMin[0]) / data.TileWorldSize));
			const int x1 = std::min(data.TilesX - 1, (int)std::floor((hi[0] + border - boundsMin[0]) / data.TileWorldSize));
			const int z0 = std::max(0, (int)std::floor((lo[1] - border - boundsMin[2]) / data.TileWorldSize));
			const int z1 = std::min(data.TilesZ - 1, (int)std::floor((hi[1] + border - boundsMin[2]) / data.TileWorldSize));
			for (int z = z0; z <= z1; z++)
			{
				for (int x = x0; x <= x1; x++)
				{
					tileTriangles[(size_t)z * data.TilesX + x].push_back(triangle);
				}
			}
		}

		rcContext context(false);
		for (int z = 0; z < data.TilesZ; z++)
		{
			for (int x = 0; x < data.TilesX; x++)
			{
				if (progress && progress->Cancel)
				{
					result.Canceled = true;
					data.Tiles.clear();
					data.Polygons = 0;
					return result;
				}
				float tileMin[3] = { boundsMin[0] + x * data.TileWorldSize, boundsMin[1], boundsMin[2] + z * data.TileWorldSize };
				float tileMax[3] = { boundsMin[0] + (x + 1) * data.TileWorldSize, boundsMax[1], boundsMin[2] + (z + 1) * data.TileWorldSize };
				NavMeshTile tile;
				std::string error;
				if (!BuildTile(context, config, input, tileTriangles[(size_t)z * data.TilesX + x], vertices, x, z, tileMin, tileMax, tile, error))
				{
					result.Error = "tile (" + std::to_string(x) + ", " + std::to_string(z) + "): " + error;
					data.Tiles.clear();
					return result;
				}
				if (!tile.Data.empty())
				{
					const dtMeshHeader* header = (const dtMeshHeader*)tile.Data.data();
					data.Polygons += header->polyCount;
					data.Tiles.push_back(std::move(tile));
				}
				if (progress)
				{
					progress->TilesDone++;
				}
			}
		}
		result.Milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		return result;
	}

	// ----------------------------------------------------------------------------------------------------------- the test

	// A closed box's triangles (its faces facing out)
	static void AddBox(Input& input, const glm::vec3& lo, const glm::vec3& hi)
	{
		const uint32_t first = (uint32_t)input.Vertices.size();
		for (int i = 0; i < 8; i++)
		{
			input.Vertices.push_back(glm::vec3(i & 1 ? hi.x : lo.x, i & 2 ? hi.y : lo.y, i & 4 ? hi.z : lo.z));
		}
		// Corners: bit 0 x, bit 1 y, bit 2 z. Each face as two triangles, counterclockwise from outside
		static const uint32_t faces[6][4] = {
			{ 2, 6, 7, 3 },  // +y (the top: walkable)
			{ 0, 1, 5, 4 },  // -y
			{ 1, 3, 7, 5 },  // +x
			{ 0, 4, 6, 2 },  // -x
			{ 4, 5, 7, 6 },  // +z
			{ 0, 2, 3, 1 },  // -z
		};
		for (const auto& f : faces)
		{
			for (uint32_t k : { f[0], f[1], f[2], f[0], f[2], f[3] })
			{
				input.Indices.push_back(first + k);
			}
		}
	}

	static float PathLength(const std::vector<glm::vec3>& corners)
	{
		float length = 0.0f;
		for (size_t i = 1; i < corners.size(); i++)
		{
			length += glm::distance(corners[i - 1], corners[i]);
		}
		return length;
	}

	bool SelfTest()
	{
		auto fail = [](const std::string& what)
		{
			Log::GetLogger()->error("Navigation self-test failed: {0}", what);
			return false;
		};

		// A 40 x 12 m floor (two tiles of 32 m), a wall 0.4 m thick across it from x = 0 to 8 at z = 6, and a 3 m high
		// platform of 4 x 4 m in a corner (walkable on top, but nothing leads up)
		Input input;
		AddBox(input, glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(40.0f, 0.0f, 12.0f));
		AddBox(input, glm::vec3(0.0f, 0.0f, 5.8f), glm::vec3(8.0f, 3.0f, 6.2f));
		AddBox(input, glm::vec3(30.0f, 0.0f, 0.0f), glm::vec3(34.0f, 3.0f, 4.0f));

		NavMeshSettings settings;
		const Result result = Build(input, settings);
		if (!result.Error.empty())
		{
			return fail("the bake: " + result.Error);
		}
		const NavMeshData& data = result.Data;
		if (data.TilesX != 2 || data.TilesZ != 1 || data.Tiles.size() != 2)
		{
			return fail("expected 2 tiles, got " + std::to_string(data.Tiles.size()) + " of a " + std::to_string(data.TilesX) +
				" x " + std::to_string(data.TilesZ) + " grid");
		}
		NavMesh navMesh;
		if (!navMesh.Load(data))
		{
			return fail("the tiles couldn't be loaded");
		}

		// Around the wall's end: about 6.3 m to it, 7.1 m back (the straight line, through the wall, is 8 m)
		std::vector<glm::vec3> corners;
		bool complete = false;
		if (!navMesh.FindPath(glm::vec3(2.0f, 0.0f, 2.0f), glm::vec3(2.0f, 0.0f, 10.0f), corners, &complete) || !complete)
		{
			return fail("no path around the wall");
		}
		const float around = PathLength(corners);
		if (corners.size() < 3 || around < 14.0f || around > 17.0f)
		{
			return fail("the path around the wall is " + std::to_string(around) + " m with " + std::to_string(corners.size()) +
				" corners (expected 14 to 17 m, at least 3 corners)");
		}
		for (const glm::vec3& c : corners)
		{
			if (c.z > 5.5f && c.z < 6.5f && c.x < 8.0f + settings.AgentRadius - 0.05f)
			{
				return fail("the path goes through the wall");
			}
		}

		// Across the two tiles: a straight walk, clear
		glm::vec3 hit;
		if (!navMesh.Raycast(glm::vec3(2.0f, 0.0f, 2.0f), glm::vec3(38.0f, 0.0f, 8.0f), &hit))
		{
			return fail("a straight walk across both tiles was blocked");
		}
		if (navMesh.Raycast(glm::vec3(2.0f, 0.0f, 2.0f), glm::vec3(2.0f, 0.0f, 10.0f), &hit) || hit.z > 6.0f)
		{
			return fail("a straight walk through the wall wasn't blocked before it");
		}

		// The platform's top is on the mesh, but no way leads up: a partial path, ending on the floor beside it
		glm::vec3 onTop;
		if (!navMesh.NearestPoint(glm::vec3(32.0f, 3.2f, 2.0f), 0.5f, onTop) || std::abs(onTop.y - 3.0f) > 0.2f)
		{
			return fail("the platform's top isn't on the mesh");
		}
		if (navMesh.FindPath(glm::vec3(20.0f, 0.0f, 8.0f), onTop, corners, &complete) && complete)
		{
			return fail("a path leads up onto the platform, which has no way up");
		}

		Log::GetLogger()->info("Navigation self-test passed ({0} tiles, {1} polygons baked in {2:.1f} ms; {3:.1f} m around the wall)",
			data.Tiles.size(), data.Polygons, result.Milliseconds, around);
		return true;
	}
}
