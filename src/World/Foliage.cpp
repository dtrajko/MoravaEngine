#include "World/Foliage.h"
#include "World/Terrain.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>


namespace
{
	// The engine's own random numbers (SplitMix64), as the terrain's: the same places from a seed with every compiler
	uint64_t Mix(uint64_t x)
	{
		x += 0x9E3779B97F4A7C15ull;
		x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
		x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
		return x ^ (x >> 31);
	}

	struct Random
	{
		uint64_t State;
		explicit Random(uint64_t seed) : State(seed) {}
		float Next() // 0 to 1
		{
			State = Mix(State);
			return (float)(State >> 40) / (float)(1ull << 24);
		}
	};

	uint64_t HashFloat(uint64_t hash, float value)
	{
		uint32_t bits;
		static_assert(sizeof(bits) == sizeof(value), "a float is 32 bits");
		std::memcpy(&bits, &value, sizeof(bits));
		return Mix(hash ^ bits);
	}
}

uint64_t FoliageSettings::GetScatterHash() const
{
	uint64_t hash = Mix(Seed);
	hash = HashFloat(hash, CellSize);
	hash = HashFloat(hash, ClearHeight);
	for (const FoliageSpecies& species : Species)
	{
		for (char c : species.Model)
		{
			hash = Mix(hash ^ (uint8_t)c);
		}
		for (float value : { species.Density, species.MinScale, species.MaxScale, species.MaxSlope, species.MinHeight, species.MaxHeight,
			species.KeepOut, species.MaxDistance, species.AlignToSlope, species.Sink })
		{
			hash = HashFloat(hash, value);
		}
	}
	return hash;
}

void FoliageSettings::Clamp()
{
	CellSize = std::clamp(CellSize, 16.0f, 512.0f);
	DrawDistance = std::max(DrawDistance, 0.0f);
	ShadowDistance = std::clamp(ShadowDistance, 0.0f, DrawDistance);
	ClearHeight = std::max(ClearHeight, 0.0f);
	for (FoliageSpecies& species : Species)
	{
		species.Density = std::clamp(species.Density, 0.0f, FoliageSpecies::MaxDensity);
		species.MinScale = std::max(species.MinScale, 0.01f);
		species.MaxScale = std::max(species.MaxScale, species.MinScale);
		species.MaxSlope = std::clamp(species.MaxSlope, 0.0f, 90.0f);
		species.MaxHeight = std::max(species.MaxHeight, species.MinHeight);
		species.KeepOut = std::max(species.KeepOut, 0.0f);
		species.MaxDistance = species.MaxDistance > 0.0f ? std::max(species.MaxDistance, species.KeepOut) : 0.0f;
		species.AlignToSlope = std::clamp(species.AlignToSlope, 0.0f, 1.0f);
		species.Sink = std::max(species.Sink, 0.0f);
	}
}

namespace Foliage
{
	uint64_t GetScatterHash(const FoliageSettings& settings, const TerrainHeights& terrain)
	{
		return Mix(settings.GetScatterHash() ^ terrain.ShapeHash);
	}

	std::shared_ptr<const FoliageInstances> Scatter(const FoliageSettings& settings, const TerrainHeights& terrain, const KeepOutQuery& keepOut)
	{
		const auto start = std::chrono::steady_clock::now();
		auto result = std::make_shared<FoliageInstances>();
		result->ScatterHash = GetScatterHash(settings, terrain);
		result->CopiesBySpecies.assign(settings.Species.size(), 0);
		if (terrain.Heights.empty())
		{
			return result;
		}

		const float half = 0.5f * terrain.Size;
		const float cellSize = settings.CellSize;
		const int cellsPerSide = std::max(1, (int)std::ceil(terrain.Size / cellSize));
		// The copies of each cell (by its index), each species' in order
		std::map<int, std::vector<FoliageInstances::Copy>> cells;
		size_t total = 0;

		for (uint32_t s = 0; s < (uint32_t)settings.Species.size(); s++)
		{
			const FoliageSpecies& species = settings.Species[s];
			if (species.Density <= 0.0f || species.Model.empty())
			{
				continue;
			}
			// A square of ground per copy, over the whole terrain: the same squares whatever the cell size, so the places
			// depend only on the species' own rules and the seed
			const float step = std::sqrt(10000.0f / species.Density);
			const int squares = (int)std::ceil(terrain.Size / step);
			const float minNormalY = std::cos(glm::radians(species.MaxSlope));
			for (int iz = 0; iz < squares; iz++)
			{
				for (int ix = 0; ix < squares; ix++)
				{
					// Each square's own random numbers: from the seed, the species and the square (not from the order of the loop)
					Random random(Mix(Mix(Mix(settings.Seed) ^ (uint64_t)(s + 1) * 0x51ED27ull) ^ ((uint64_t)(uint32_t)iz << 32 | (uint32_t)ix)));
					const float x = -half + (ix + random.Next()) * step;
					const float z = -half + (iz + random.Next()) * step;
					const float yaw = random.Next() * glm::two_pi<float>();
					const float scale = species.MinScale + (species.MaxScale - species.MinScale) * random.Next();
					if (x > half || z > half)
					{
						continue;
					}
					const glm::vec3 normal = terrain.GetNormal(x, z);
					if (normal.y < minNormalY)
					{
						continue;
					}
					const float y = terrain.GetHeight(x, z);
					if (y < species.MinHeight || y > species.MaxHeight)
					{
						continue;
					}
					const glm::vec3 ground(x, y, z);
					if (keepOut && species.MaxDistance > 0.0f && !keepOut(ground, species.MaxDistance, settings.ClearHeight))
					{
						continue; // too far from everything (from the track)
					}
					if (keepOut && keepOut(ground, species.KeepOut, settings.ClearHeight))
					{
						result->KeptOut++;
						continue;
					}
					if (total >= FoliageSettings::MaxCopies)
					{
						break;
					}

					// Upright, or tilted part of the way to the ground's normal; turned around that up; sunk into the ground
					const glm::vec3 up = glm::normalize(glm::mix(glm::vec3(0.0f, 1.0f, 0.0f), normal, species.AlignToSlope));
					const glm::vec3 axis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), up);
					const float axisLength = glm::length(axis);
					const glm::quat tilt = axisLength > 1e-6f ? glm::angleAxis(std::asin(std::min(axisLength, 1.0f)), axis / axisLength) : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
					const glm::quat turn = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), ground - up * species.Sink) * glm::mat4_cast(tilt * turn) *
						glm::scale(glm::mat4(1.0f), glm::vec3(scale));
					const int cx = std::clamp((int)std::floor((x + half) / cellSize), 0, cellsPerSide - 1);
					const int cz = std::clamp((int)std::floor((z + half) / cellSize), 0, cellsPerSide - 1);
					cells[cz * cellsPerSide + cx].push_back({ transform, s });
					result->CopiesBySpecies[s]++;
					total++;
				}
			}
		}

		// The cells in order, each one's copies by species (the renderer draws a species' copies of a cell in one call)
		result->Copies.reserve(total);
		for (auto& [index, copies] : cells)
		{
			std::stable_sort(copies.begin(), copies.end(), [](const FoliageInstances::Copy& a, const FoliageInstances::Copy& b) { return a.Species < b.Species; });
			FoliageInstances::Cell cell;
			const int cx = index % cellsPerSide, cz = index / cellsPerSide;
			cell.Min = glm::vec2(-half + cx * cellSize, -half + cz * cellSize);
			cell.Max = cell.Min + glm::vec2(cellSize);
			cell.First = (uint32_t)result->Copies.size();
			cell.Count = (uint32_t)copies.size();
			result->Copies.insert(result->Copies.end(), copies.begin(), copies.end());
			result->Cells.push_back(cell);
		}
		result->Milliseconds = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
		return result;
	}
}
