#include "Terrain.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <queue>
#include <random>
#include <thread>


namespace
{
	uint64_t HashValue(uint64_t hash, const void* data, size_t size)
	{
		const uint8_t* bytes = static_cast<const uint8_t*>(data);
		for (size_t i = 0; i < size; i++)
		{
			hash = (hash ^ bytes[i]) * 1099511628211ull; // FNV-1a
		}
		return hash;
	}

	template<typename T>
	uint64_t HashOf(uint64_t hash, const T& value)
	{
		return HashValue(hash, &value, sizeof(T));
	}

	// The engine's own random numbers (SplitMix64): the same sequence from a seed with every compiler
	uint64_t NextRandom(uint64_t& state)
	{
		uint64_t z = (state += 0x9E3779B97F4A7C15ull);
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
		return z ^ (z >> 31);
	}

	// Ken Perlin's gradient noise in 2D, on a permutation of 0..255 made from the seed (its own shuffle: see Terrain.h)
	class GradientNoise
	{
	public:
		explicit GradientNoise(uint64_t seed)
		{
			uint64_t state = seed;
			for (int i = 0; i < 256; i++)
			{
				m_Permutation[i] = (uint8_t)i;
			}
			for (int i = 255; i > 0; i--)
			{
				const int j = (int)(NextRandom(state) % (uint64_t)(i + 1));
				std::swap(m_Permutation[i], m_Permutation[j]);
			}
			for (int i = 0; i < 256; i++)
			{
				m_Permutation[256 + i] = m_Permutation[i];
			}
		}

		// About -1 to 1, smooth, 0 at the integer points
		float Sample(float x, float y) const
		{
			const float fx = std::floor(x), fy = std::floor(y);
			const int ix = (int)fx & 255, iy = (int)fy & 255;
			const float dx = x - fx, dy = y - fy;
			const float u = Fade(dx), v = Fade(dy);
			const int a = m_Permutation[ix] + iy, b = m_Permutation[ix + 1] + iy;
			const float n00 = Gradient(m_Permutation[a], dx, dy);
			const float n10 = Gradient(m_Permutation[b], dx - 1.0f, dy);
			const float n01 = Gradient(m_Permutation[a + 1], dx, dy - 1.0f);
			const float n11 = Gradient(m_Permutation[b + 1], dx - 1.0f, dy - 1.0f);
			const float nx0 = n00 + u * (n10 - n00);
			const float nx1 = n01 + u * (n11 - n01);
			return (nx0 + v * (nx1 - nx0)) * 1.4142135f;
		}

	private:
		static float Fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

		// One of 8 directions, by the hash's low bits
		static float Gradient(uint8_t hash, float x, float y)
		{
			switch (hash & 7)
			{
				case 0: return x + y;
				case 1: return -x + y;
				case 2: return x - y;
				case 3: return -x - y;
				case 4: return x * 1.4142135f;
				case 5: return -x * 1.4142135f;
				case 6: return y * 1.4142135f;
				default: return -y * 1.4142135f;
			}
		}

		std::array<uint8_t, 512> m_Permutation{};
	};

	// The layers of noise at a point, 0 to 1 about (before the terrain's own range is found)
	struct LayeredNoise
	{
		const GradientNoise* Noise = nullptr;
		std::vector<glm::vec2> Offsets; // each layer somewhere else in the noise, so their zeros don't line up
		uint32_t Octaves = 1;
		float Frequency = 1.0f, Roughness = 0.5f, Ridges = 0.0f;

		// The first two layers make the hills; the smaller ones add detail, more of it the higher the hills already are
		// there (smooth lowlands, rugged heights), and their ridges count half
		float Sample(float x, float z) const
		{
			float frequency = Frequency, amplitude = 1.0f, sum = 0.0f, total = 0.0f;
			// Each layer turned by about 37 degrees from the last: their grids don't add up to lines along the axes
			const float c = 0.7986f, s = 0.6018f;
			float px = x, pz = z;
			for (uint32_t o = 0; o < Octaves; o++)
			{
				const float n = Noise->Sample(px * frequency + Offsets[o].x, pz * frequency + Offsets[o].y);
				const float round = n * 0.5f + 0.5f;
				const float ridge = 1.0f - std::fabs(n);
				const float ridges = o < 2 ? Ridges : Ridges * 0.5f;
				const float detail = o < 2 || total <= 0.0f ? 1.0f : 0.2f + 0.8f * std::clamp(sum / total, 0.0f, 1.0f);
				sum += (round + (ridge * ridge - round) * ridges) * amplitude * detail;
				total += amplitude;
				amplitude *= Roughness;
				frequency *= 2.0f;
				const float rx = c * px - s * pz;
				pz = s * px + c * pz;
				px = rx;
			}
			return total > 0.0f ? sum / total : 0.0f;
		}
	};

	float SmoothStep(float edge0, float edge1, float x)
	{
		const float t = std::clamp((x - edge0) / std::max(edge1 - edge0, 1e-6f), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	// Runs work(first, last) over 0..count on the cores (rows of the terrain: each row by one thread, so the result doesn't
	// depend on how many there are)
	template<typename F>
	void ParallelRows(uint32_t count, F&& work)
	{
		const uint32_t threads = std::clamp(std::thread::hardware_concurrency(), 1u, 16u);
		std::vector<std::thread> pool;
		const uint32_t perThread = (count + threads - 1) / threads;
		for (uint32_t t = 0; t < threads; t++)
		{
			const uint32_t first = t * perThread, last = std::min(count, first + perThread);
			if (first < last)
			{
				pool.emplace_back([&work, first, last] { work(first, last); });
			}
		}
		for (std::thread& thread : pool)
		{
			thread.join();
		}
	}
}

uint64_t TerrainSettings::GetShapeHash() const
{
	uint64_t hash = 14695981039346656037ull;
	hash = HashOf(hash, Seed);
	hash = HashOf(hash, Size);
	hash = HashOf(hash, Height);
	hash = HashOf(hash, FeatureSize);
	hash = HashOf(hash, Octaves);
	hash = HashOf(hash, Roughness);
	hash = HashOf(hash, Ridges);
	hash = HashOf(hash, Valleys);
	hash = HashOf(hash, RimHeight);
	hash = HashOf(hash, RimWidth);
	hash = HashOf(hash, FlatCenter);
	hash = HashOf(hash, FlatRadius);
	hash = HashOf(hash, FlatBlend);
	hash = HashOf(hash, Spacing);
	hash = HashOf(hash, MaxVertices);
	hash = HashOf(hash, GuideClearance);
	hash = HashOf(hash, GuideShoulder);
	hash = HashOf(hash, GuideRise);
	hash = HashOf(hash, GuideHills);
	hash = HashOf(hash, GuideHillSize);
	hash = HashOf(hash, GuideBand);
	hash = HashOf(hash, GuideBlend);
	hash = HashOf(hash, GuideRaise);
	hash = HashOf(hash, GuideDetail);
	hash = HashOf(hash, GuideRoom);
	return hash;
}

void TerrainSettings::Clamp()
{
	Size = std::clamp(std::round(Size / ChunkSize) * ChunkSize, MinSize, MaxSize);
	Height = std::clamp(Height, 0.0f, 2000.0f);
	FeatureSize = std::clamp(FeatureSize, 8.0f, 20000.0f);
	Octaves = std::clamp(Octaves, 1u, 10u);
	Roughness = std::clamp(Roughness, 0.0f, 1.0f);
	Ridges = std::clamp(Ridges, 0.0f, 1.0f);
	Valleys = std::clamp(Valleys, 0.25f, 4.0f);
	RimHeight = std::clamp(RimHeight, 0.0f, 4000.0f);
	RimWidth = std::clamp(RimWidth, 1.0f, Size * 0.5f);
	FlatRadius = std::clamp(FlatRadius, 0.0f, Size * 0.5f);
	FlatBlend = std::clamp(FlatBlend, 0.0f, Size * 0.5f);
	Spacing = Spacing >= 3.0f ? 4.0f : Spacing >= 1.5f ? 2.0f : 1.0f; // a chunk (64 m) a whole number of cells at every level
	RockSlope = std::clamp(RockSlope, 0.0f, 90.0f);
	Friction = std::clamp(Friction, 0.0f, 2.0f);
	LodTolerance = std::clamp(LodTolerance, 0.0f, 20.0f);
	GuideClearance = std::clamp(GuideClearance, 0.0f, 100.0f);
	GuideShoulder = std::clamp(GuideShoulder, 0.0f, 1000.0f);
	GuideRise = std::clamp(GuideRise, 1.0f, 2000.0f);
	GuideHills = std::clamp(GuideHills, 0.0f, 1000.0f);
	GuideHillSize = std::clamp(GuideHillSize, 8.0f, 20000.0f);
	GuideBand = std::clamp(GuideBand, 0.0f, Size);
	GuideBlend = std::clamp(GuideBlend, 1.0f, Size);
	GuideRaise = std::clamp(GuideRaise, -1000.0f, 1000.0f);
	GuideDetail = std::clamp(GuideDetail, 0.0f, 100.0f);
	GuideRoom = std::clamp(GuideRoom, 0.0f, Size);
}

// ---- TerrainHeights

uint32_t TerrainLods::LevelAt(const Chunk& chunk, float distance, float tolerance)
{
	// The error allowed grows with the distance (as the size of a meter on the screen shrinks); at least 1 m away, so flat
	// ground under the camera is still its coarsest level
	const float allowed = tolerance * std::max(distance, 1.0f) / 100.0f;
	uint32_t level = 0;
	for (uint32_t l = 1; l < LevelCount; l++)
	{
		if (chunk.Error[l] <= allowed)
		{
			level = l;
		}
	}
	return level;
}

float TerrainHeights::GetHeight(float x, float z) const
{
	if (SampleCount < 2)
	{
		return 0.0f;
	}
	const float half = Size * 0.5f;
	const float max = (float)(SampleCount - 1);
	const float gx = std::clamp((x + half) / Spacing, 0.0f, max), gz = std::clamp((z + half) / Spacing, 0.0f, max);
	const uint32_t ix = std::min((uint32_t)gx, SampleCount - 2), iz = std::min((uint32_t)gz, SampleCount - 2);
	const float fx = gx - (float)ix, fz = gz - (float)iz;
	const float ha = Heights[iz * SampleCount + ix], hb = Heights[iz * SampleCount + ix + 1];
	const float hc = Heights[(iz + 1) * SampleCount + ix], hd = Heights[(iz + 1) * SampleCount + ix + 1];
	// The square's two triangles, split from a (-x -z) to d (+x +z), as BuildModelData and physics split it
	return fz > fx ? ha + fx * (hd - hc) + fz * (hc - ha) : ha + fx * (hb - ha) + fz * (hd - hb);
}

glm::vec3 TerrainHeights::GetNormal(float x, float z) const
{
	const float e = Spacing;
	const float dx = GetHeight(x + e, z) - GetHeight(x - e, z);
	const float dz = GetHeight(x, z + e) - GetHeight(x, z - e);
	return glm::normalize(glm::vec3(-dx, 2.0f * e, -dz));
}

bool TerrainHeights::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, float& t) const
{
	if (SampleCount < 2)
	{
		return false;
	}
	// The part of the ray inside the terrain's box
	const float half = Size * 0.5f;
	const glm::vec3 boxMin(-half, MinHeight - 0.01f, -half), boxMax(half, MaxHeight + 0.01f, half);
	float enter = 0.0f, exit = maxDistance;
	for (int axis = 0; axis < 3; axis++)
	{
		if (std::fabs(direction[axis]) < 1e-12f)
		{
			if (origin[axis] < boxMin[axis] || origin[axis] > boxMax[axis])
			{
				return false;
			}
			continue;
		}
		float t0 = (boxMin[axis] - origin[axis]) / direction[axis], t1 = (boxMax[axis] - origin[axis]) / direction[axis];
		if (t0 > t1)
		{
			std::swap(t0, t1);
		}
		enter = std::max(enter, t0);
		exit = std::min(exit, t1);
		if (enter > exit)
		{
			return false;
		}
	}

	// Steps of half a sample's spacing across the ground, until the ray is below it; then the crossing, halved down
	auto above = [&](float at)
	{
		const glm::vec3 p = origin + direction * at;
		return p.y - GetHeight(p.x, p.z);
	};
	const float across = glm::length(glm::vec2(direction.x, direction.z));
	const float step = across > 1e-9f ? 0.5f * Spacing / across : exit - enter;
	float previous = enter, previousAbove = above(enter);
	if (previousAbove <= 0.0f)
	{
		t = enter; // starts under the ground (or on it)
		return true;
	}
	for (float at = std::min(enter + step, exit); ; at = std::min(at + step, exit))
	{
		const float now = above(at);
		if (now <= 0.0f)
		{
			float low = previous, high = at;
			for (int i = 0; i < 20; i++)
			{
				const float middle = 0.5f * (low + high);
				(above(middle) > 0.0f ? low : high) = middle;
			}
			t = high;
			return true;
		}
		previous = at;
		if (at >= exit)
		{
			return false;
		}
	}
}

// ---- The guides

namespace
{
	// The point of a triangle nearest to p in x and z (the triangle seen from above): how far it is and the triangle's height
	// there. False: the triangle is edge-on from above (a wall's side), no surface to follow
	bool NearestFromAbove(const std::array<glm::vec3, 3>& triangle, const glm::vec2& p, float& distance, float& height)
	{
		const glm::vec2 a(triangle[0].x, triangle[0].z), b(triangle[1].x, triangle[1].z), c(triangle[2].x, triangle[2].z);
		const glm::vec2 ab = b - a, ac = c - a;
		const float area = ab.x * ac.y - ab.y * ac.x;
		if (std::fabs(area) < 1e-6f)
		{
			return false;
		}
		// Inside: the height on the plane
		const glm::vec2 ap = p - a;
		const float v = (ap.x * ac.y - ap.y * ac.x) / area, w = (ab.x * ap.y - ab.y * ap.x) / area;
		if (v >= 0.0f && w >= 0.0f && v + w <= 1.0f)
		{
			distance = 0.0f;
			height = triangle[0].y + v * (triangle[1].y - triangle[0].y) + w * (triangle[2].y - triangle[0].y);
			return true;
		}
		// Outside: the nearest point of its edges
		distance = std::numeric_limits<float>::max();
		for (int e = 0; e < 3; e++)
		{
			const glm::vec2 from(triangle[e].x, triangle[e].z), to(triangle[(e + 1) % 3].x, triangle[(e + 1) % 3].z);
			const glm::vec2 along = to - from;
			const float lengthSquared = glm::dot(along, along);
			const float t = lengthSquared > 1e-12f ? std::clamp(glm::dot(p - from, along) / lengthSquared, 0.0f, 1.0f) : 0.0f;
			const float d = glm::length(p - (from + along * t));
			if (d < distance)
			{
				distance = d;
				height = triangle[e].y + t * (triangle[(e + 1) % 3].y - triangle[e].y);
			}
		}
		return true;
	}

	// Each row's values averaged over radius samples either side (twice: a tent, smooth), then each column's
	void Blur(std::vector<float>& values, uint32_t count, int radius)
	{
		if (radius < 1)
		{
			return;
		}
		auto pass = [&](bool rows)
		{
			std::vector<float> source = values;
			ParallelRows(count, [&](uint32_t first, uint32_t last)
			{
				std::vector<double> sums(count + 1);
				for (uint32_t line = first; line < last; line++)
				{
					auto at = [&](uint32_t i) -> size_t { return rows ? (size_t)line * count + i : (size_t)i * count + line; };
					sums[0] = 0.0;
					for (uint32_t i = 0; i < count; i++)
					{
						sums[i + 1] = sums[i] + source[at(i)];
					}
					for (uint32_t i = 0; i < count; i++)
					{
						const int from = std::max((int)i - radius, 0), to = std::min((int)i + radius, (int)count - 1);
						values[at(i)] = (float)((sums[to + 1] - sums[from]) / (double)(to - from + 1));
					}
				}
			});
		};
		for (int repeat = 0; repeat < 2; repeat++)
		{
			pass(true);
			pass(false);
		}
	}

	// The ground near the guides (TerrainSettings' Guide values), over the terrain's own heights h (the terrain's space)
	void FollowGuides(const TerrainSettings& settings, const TerrainGuides& guides, TerrainHeights& heights)
	{
		const uint32_t count = heights.SampleCount;
		std::vector<float>& h = heights.Heights;
		const float half = settings.Size * 0.5f, spacing = settings.Spacing;
		const size_t samples = (size_t)count * count;
		auto position = [&](uint32_t x, uint32_t z) { return glm::vec2(-half + x * spacing, -half + z * spacing); };

		// The guide's surface at each sample under it or near it: the lowest it is within a sample and a half (so the ground
		// between two samples, a straight line, stays below a road that slopes or banks away from them)
		const float reach = 1.5f * spacing;
		constexpr float none = std::numeric_limits<float>::max();
		std::vector<float> surface(samples, none);
		for (const std::array<glm::vec3, 3>& triangle : guides.Triangles)
		{
			const float minX = std::min({ triangle[0].x, triangle[1].x, triangle[2].x }) - reach;
			const float maxX = std::max({ triangle[0].x, triangle[1].x, triangle[2].x }) + reach;
			const float minZ = std::min({ triangle[0].z, triangle[1].z, triangle[2].z }) - reach;
			const float maxZ = std::max({ triangle[0].z, triangle[1].z, triangle[2].z }) + reach;
			if (maxX < -half || minX > half || maxZ < -half || minZ > half)
			{
				continue; // beyond the terrain
			}
			const uint32_t x0 = (uint32_t)std::clamp(std::ceil((minX + half) / spacing), 0.0f, (float)(count - 1));
			const uint32_t x1 = (uint32_t)std::clamp(std::floor((maxX + half) / spacing), 0.0f, (float)(count - 1));
			const uint32_t z0 = (uint32_t)std::clamp(std::ceil((minZ + half) / spacing), 0.0f, (float)(count - 1));
			const uint32_t z1 = (uint32_t)std::clamp(std::floor((maxZ + half) / spacing), 0.0f, (float)(count - 1));
			for (uint32_t z = z0; z <= z1; z++)
			{
				for (uint32_t x = x0; x <= x1; x++)
				{
					float distance = 0.0f, height = 0.0f;
					if (NearestFromAbove(triangle, position(x, z), distance, height) && distance <= reach)
					{
						float& value = surface[(size_t)z * count + x];
						value = std::min(value, height);
					}
				}
			}
		}

		// The nearest of those samples to every sample (two sweeps over the grid, each passing on its neighbors' nearest:
		// within a fraction of a sample of the true nearest)
		std::vector<int32_t> nearest(samples, -1);
		bool any = false;
		for (size_t i = 0; i < samples; i++)
		{
			if (surface[i] != none)
			{
				nearest[i] = (int32_t)i;
				any = true;
			}
		}
		if (!any)
		{
			return; // no guide over the terrain
		}
		auto distanceSquared = [&](size_t i, int32_t to)
		{
			const int64_t dx = (int64_t)(i % count) - (int64_t)(to % count), dz = (int64_t)(i / count) - (int64_t)(to / count);
			return dx * dx + dz * dz;
		};
		auto consider = [&](uint32_t x, uint32_t z, int ox, int oz)
		{
			const int nx = (int)x + ox, nz = (int)z + oz;
			if (nx < 0 || nz < 0 || nx >= (int)count || nz >= (int)count)
			{
				return;
			}
			const size_t i = (size_t)z * count + x;
			const int32_t candidate = nearest[(size_t)nz * count + nx];
			if (candidate >= 0 && (nearest[i] < 0 || distanceSquared(i, candidate) < distanceSquared(i, nearest[i])))
			{
				nearest[i] = candidate;
			}
		};
		for (int sweep = 0; sweep < 2; sweep++)
		{
			for (uint32_t z = 0; z < count; z++)
			{
				for (uint32_t x = 0; x < count; x++)
				{
					consider(x, z, -1, 0); consider(x, z, -1, -1); consider(x, z, 0, -1); consider(x, z, 1, -1);
				}
				for (uint32_t x = count; x-- > 0;)
				{
					consider(x, z, 1, 0);
				}
			}
			for (uint32_t z = count; z-- > 0;)
			{
				for (uint32_t x = count; x-- > 0;)
				{
					consider(x, z, 1, 0); consider(x, z, 1, 1); consider(x, z, 0, 1); consider(x, z, -1, 1);
				}
				for (uint32_t x = 0; x < count; x++)
				{
					consider(x, z, -1, 0);
				}
			}
		}

		// The guide's height at the nearest point, and smoothed: where two parts of a road at different heights are near
		// each other, the ground between them has no step where the nearest changes from one to the other
		heights.Ceiling.assign(samples, none);
		heights.Floor.assign(samples, -none);
		std::vector<float> guide(samples), smooth(samples);
		for (size_t i = 0; i < samples; i++)
		{
			guide[i] = smooth[i] = surface[nearest[i]];
		}
		Blur(smooth, count, (int)std::lround((settings.GuideShoulder + settings.GuideRise) * 0.5f / spacing));

		// The room for hills: from each height, straight on away from its nearest guide, how far until a guide again (the
		// ground between two parts of a road); 1 with room enough (GuideRoom), less where it is narrower. Smoothed: where
		// the other part ends (around a hairpin's end) the ground has no step
		std::vector<float> room(samples, 1.0f);
		if (settings.GuideRoom > 0.0f)
		{
			const float reachOut = settings.GuideBand + settings.GuideBlend;
			ParallelRows(count, [&](uint32_t first, uint32_t last)
			{
				for (uint32_t z = first; z < last; z++)
				{
					for (uint32_t x = 0; x < count; x++)
					{
						const size_t i = (size_t)z * count + x;
						const int32_t seed = nearest[i];
						const float sx = (float)(seed % count), sz = (float)(seed / count);
						const float dx = (float)x - sx, dz = (float)z - sz;
						const float length = std::sqrt(dx * dx + dz * dz);
						if (length < 1.0f || length * spacing > reachOut)
						{
							continue;
						}
						// Straight on, a sample at a time, up to GuideRoom from the guide behind
						const float ux = dx / length, uz = dz / length;
						float gap = settings.GuideRoom;
						for (float t = 1.0f; (length + t) * spacing < settings.GuideRoom; t += 1.0f)
						{
							const int px = (int)std::lround((float)x + ux * t), pz = (int)std::lround((float)z + uz * t);
							if (px < 0 || pz < 0 || px >= (int)count || pz >= (int)count)
							{
								break;
							}
							if (surface[(size_t)pz * count + px] != none)
							{
								gap = (length + t) * spacing;
								break;
							}
						}
						room[i] = SmoothStep(0.4f * settings.GuideRoom, settings.GuideRoom, gap);
					}
				}
			});
			Blur(room, count, (int)std::lround(settings.GuideRise * 0.5f / spacing));
		}

		// The hills near the guides: their own noise, -1 to 1
		GradientNoise noise(((uint64_t)settings.Seed << 1) ^ 0x9E3779B97F4A7C15ull);
		const float frequency = 1.0f / settings.GuideHillSize;
		ParallelRows(count, [&](uint32_t first, uint32_t last)
		{
			for (uint32_t z = first; z < last; z++)
			{
				for (uint32_t x = 0; x < count; x++)
				{
					const size_t i = (size_t)z * count + x;
					const float distance = std::sqrt((float)distanceSquared(i, nearest[i])) * spacing;
					const glm::vec2 p = position(x, z);
					// Just below the guide, then below its smoothed height further out
					const float level = guide[i] + (smooth[i] - guide[i]) * SmoothStep(0.0f, settings.GuideShoulder + settings.GuideRise, distance)
						- settings.GuideClearance;
					const float n = std::clamp((noise.Sample(p.x * frequency + 17.3f, p.y * frequency + 41.9f) +
						0.5f * noise.Sample(p.x * frequency * 2.3f + 63.1f, p.y * frequency * 2.3f + 5.7f)) / 1.5f * 1.4f, -1.0f, 1.0f);
					// Past the shoulder the ground rises by GuideRaise (a valley under the guide, hills higher than it beyond), and
					// a hill above that level rises with it (a road through a cutting); one below it dips under the guide too (a
					// road on its supports)
					const float rise = SmoothStep(settings.GuideShoulder, settings.GuideShoulder + settings.GuideRise, distance);
					const float raised = level + settings.GuideRaise * rise * room[i];
					float value = n <= 0.0f ? raised + settings.GuideHills * n :
						raised + settings.GuideHills * n * rise * (0.5f + 0.5f * room[i]);
					// Beyond the band, the terrain's own hills
					value += (h[i] - value) * SmoothStep(settings.GuideBand, settings.GuideBand + settings.GuideBlend, distance);
					// Never above the guide's surface where it is over the ground
					if (surface[i] != none)
					{
						value = std::min(value, surface[i] - settings.GuideClearance);
					}
					h[i] = value;
					// And the drawn ground's limits: under the guide half way down to the ground physics has; within the band
					// no lower than GuideDetail under it
					heights.Ceiling[i] = surface[i] == none ? none : surface[i] - 0.5f * settings.GuideClearance;
					heights.Floor[i] = settings.GuideDetail > 0.0f && distance <= settings.GuideBand ? value - settings.GuideDetail : -none;
				}
			}
		});
	}
}

// ---- Terrain

namespace Terrain
{
	uint64_t GetShapeHash(const TerrainSettings& settings, uint64_t guidesHash)
	{
		const uint64_t hash = settings.GetShapeHash();
		return guidesHash == 0 ? hash : HashOf(hash, guidesHash);
	}

	std::shared_ptr<const TerrainHeights> GenerateHeights(const TerrainSettings& input, const TerrainGuides* guides)
	{
		const auto start = std::chrono::steady_clock::now();
		TerrainSettings settings = input;
		settings.Clamp();

		auto heights = std::make_shared<TerrainHeights>();
		heights->Spacing = settings.Spacing;
		heights->Size = settings.Size;
		heights->SampleCount = (uint32_t)std::lround(settings.Size / settings.Spacing) + 1;
		const bool guided = guides && !guides->Triangles.empty();
		heights->ShapeHash = GetShapeHash(input, guides ? guides->Hash : 0);
		const uint32_t count = heights->SampleCount;
		heights->Heights.resize((size_t)count * count);

		GradientNoise noise(((uint64_t)settings.Seed << 1) ^ 0x5DEECE66Dull);
		LayeredNoise layers;
		layers.Noise = &noise;
		layers.Octaves = settings.Octaves;
		layers.Frequency = 1.0f / settings.FeatureSize;
		layers.Roughness = settings.Roughness;
		layers.Ridges = settings.Ridges;
		uint64_t state = settings.Seed;
		for (uint32_t o = 0; o < settings.Octaves; o++)
		{
			const float ox = (float)(NextRandom(state) % 100000) * 0.01f;
			const float oz = (float)(NextRandom(state) % 100000) * 0.01f;
			layers.Offsets.push_back(glm::vec2(ox, oz));
		}

		const float half = settings.Size * 0.5f;
		const float spacing = settings.Spacing;
		std::vector<float>& h = heights->Heights;
		ParallelRows(count, [&](uint32_t first, uint32_t last)
		{
			for (uint32_t z = first; z < last; z++)
			{
				for (uint32_t x = 0; x < count; x++)
				{
					h[(size_t)z * count + x] = layers.Sample(-half + x * spacing, -half + z * spacing);
				}
			}
		});

		// The noise's own range to 0..1: Height is then the terrain's, whatever the layers add up to. Valleys bends it: wide
		// lowlands, steep peaks
		const auto [lowest, highest] = std::minmax_element(h.begin(), h.end());
		const float low = *lowest, range = std::max(*highest - *lowest, 1e-6f);
		const float flatHeight = std::pow(std::clamp((layers.Sample(settings.FlatCenter.x, settings.FlatCenter.y) - low) / range, 0.0f, 1.0f),
			settings.Valleys) * settings.Height;
		ParallelRows(count, [&](uint32_t first, uint32_t last)
		{
			for (uint32_t z = first; z < last; z++)
			{
				for (uint32_t x = 0; x < count; x++)
				{
					float& value = h[(size_t)z * count + x];
					const float noise = (value - low) / range;
					value = std::pow(noise, settings.Valleys) * settings.Height;
					// The rim: towards the edges (a rounded square: no sharp corners) the ground rises to a crest above the
					// highest hills inside (Height), by RimHeight more where the noise is high, so it is a line of hills
					// rather than a wall, and no gap in it is low enough to show what is beyond
					if (settings.RimHeight > 0.0f)
					{
						const float px = std::fabs(-half + x * spacing) / half, pz = std::fabs(-half + z * spacing) / half;
						const float edge = std::pow(std::pow(px, 6.0f) + std::pow(pz, 6.0f), 1.0f / 6.0f) * half; // meters from the center
						const float rise = SmoothStep(half - settings.RimWidth, half, edge);
						const float crest = settings.Height * 0.9f + settings.RimHeight * (0.4f + 1.2f * noise);
						value += (std::max(value, crest) - value) * rise * rise;
					}
					// The flat area: level at the height the hills have at its center, rising back into them around it
					if (settings.FlatRadius > 0.0f || settings.FlatBlend > 0.0f)
					{
						const float distance = glm::length(glm::vec2(-half + x * spacing, -half + z * spacing) - settings.FlatCenter);
						const float hills = SmoothStep(settings.FlatRadius, settings.FlatRadius + settings.FlatBlend, distance);
						value = flatHeight + (value - flatHeight) * hills;
					}
				}
			}
		});

		// The lowest point at 0: the look's heights (sand, rock) count from it
		const auto [newLowest, newHighest] = std::minmax_element(h.begin(), h.end());
		const float shift = *newLowest;
		for (float& value : h)
		{
			value -= shift;
		}
		heights->MinHeight = 0.0f;
		heights->MaxHeight = *newHighest - shift;

		// The guides are in the terrain's space: the ground near them is at their heights, wherever its lowest point is
		if (guided)
		{
			FollowGuides(settings, *guides, *heights);
			const auto [guidedLowest, guidedHighest] = std::minmax_element(h.begin(), h.end());
			heights->MinHeight = *guidedLowest;
			heights->MaxHeight = *guidedHighest;
		}
		heights->Milliseconds = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
		return heights;
	}

	ModelData BuildModelData(const TerrainHeights& heights, TerrainLods* lods, uint32_t maxVertices)
	{
		ModelData data;
		const uint32_t count = heights.SampleCount;
		if (count < 2)
		{
			return data;
		}
		const uint32_t cellsPerChunk = TerrainSettings::ChunkCells;
		const uint32_t cells = count - 1;
		const uint32_t chunks = (cells + cellsPerChunk - 1) / cellsPerChunk;
		const float half = heights.Size * 0.5f;
		const float spacing = heights.Spacing;
		auto at = [&](int x, int z)
		{
			x = std::clamp(x, 0, (int)count - 1);
			z = std::clamp(z, 0, (int)count - 1);
			return heights.Heights[(size_t)z * count + x];
		};
		// Under and beside a guide: the highest the drawn ground may be (TerrainHeights::Ceiling); within its band the lowest (Floor)
		const bool hasCeiling = heights.Ceiling.size() == heights.Heights.size();
		auto ceiling = [&](int x, int z) { return heights.Ceiling[(size_t)z * count + x]; };
		const bool hasFloor = heights.Floor.size() == heights.Heights.size();
		auto floorAt = [&](int x, int z) { return heights.Floor[(size_t)z * count + x]; };
		constexpr float Forbidden = std::numeric_limits<float>::infinity(); // a level's error there: never drawn
		// A level's spacing in heights, when it fits the chunk (else the level before it is used again)
		auto levelFits = [](uint32_t level, uint32_t w, uint32_t d) { const uint32_t step = 1u << level; return w % step == 0 && d % step == 0; };

		// Each chunk's shape first: every level's error (the most its triangles stray from the heights, each square split
		// from its corner at -x -z to the one at +x +z as the triangles are), and the level of the chunk's own mesh: the
		// coarsest exact one (within ExactError), or the one a vertex budget gives it (below). Flat or evenly sloped ground
		// is then a few vertices and two triangles in the mesh itself, for every pass that draws it, with the levels of
		// detail or without them (the probes' capture)
		struct ChunkShape
		{
			uint32_t W = 0, D = 0; // cells
			uint32_t Base = 0;     // the level of the mesh's own triangles and vertices
			float Error[TerrainLods::LevelCount] = {};
			bool Fits[TerrainLods::LevelCount] = {};
		};
		const float ExactError = 0.005f; // meters
		std::vector<ChunkShape> shapes((size_t)chunks * chunks);
		ParallelRows(chunks, [&](uint32_t first, uint32_t last)
		{
			for (uint32_t cz = first; cz < last; cz++)
			{
				for (uint32_t cx = 0; cx < chunks; cx++)
				{
					ChunkShape& shape = shapes[(size_t)cz * chunks + cx];
					const uint32_t x0 = cx * cellsPerChunk, z0 = cz * cellsPerChunk;
					const uint32_t w = std::min(cellsPerChunk, cells - x0), d = std::min(cellsPerChunk, cells - z0);
					shape.W = w;
					shape.D = d;
					shape.Fits[0] = true;
					for (uint32_t level = 1; level < TerrainLods::LevelCount; level++)
					{
						shape.Fits[level] = levelFits(level, w, d);
						if (!shape.Fits[level])
						{
							shape.Error[level] = shape.Error[level - 1];
							continue;
						}
						const uint32_t step = 1u << level;
						float error = 0.0f;
						bool outsideLimits = false;
						for (uint32_t z = 0; z <= d; z++)
						{
							for (uint32_t x = 0; x <= w; x++)
							{
								const uint32_t qx = std::min(x / step * step, w - step), qz = std::min(z / step * step, d - step);
								const float u = (float)(x - qx) / step, v = (float)(z - qz) / step;
								const int gx = (int)(x0 + qx), gz = (int)(z0 + qz), gs = (int)step;
								const float ha = at(gx, gz), hb = at(gx + gs, gz), hc = at(gx, gz + gs), he = at(gx + gs, gz + gs);
								const float level_h = v >= u ? ha + u * (he - hc) + v * (hc - ha) : ha + u * (hb - ha) + v * (he - hb);
								error = std::max(error, std::fabs(level_h - at((int)(x0 + x), (int)(z0 + z))));
								outsideLimits = outsideLimits || (hasCeiling && level_h > ceiling((int)(x0 + x), (int)(z0 + z))) ||
									(hasFloor && level_h < floorAt((int)(x0 + x), (int)(z0 + z)));
							}
						}
						shape.Error[level] = outsideLimits ? Forbidden : std::max(error, shape.Error[level - 1]); // never less than a finer level's
						if (shape.Error[level] <= ExactError)
						{
							shape.Base = level;
						}
					}
				}
			}
		});

		// A vertex budget (maxVertices): each chunk starts as its coarsest level, then the chunk whose triangles stray
		// furthest from the heights gets its next finer level, again and again while the budget lasts (never finer than
		// its exact level). The vertices go where the ground needs them: steep hills get detail, gentle ones stay coarse
		auto vertexCost = [&](const ChunkShape& shape, uint32_t level)
		{
			const uint32_t step = 1u << level, gw = shape.W / step, gd = shape.D / step;
			return (size_t)(gw + 1) * (gd + 1) + (lods ? (size_t)2 * (gw + 1) + (size_t)2 * (gd + 1) : 0);
		};
		if (maxVertices > 0)
		{
			size_t total = 0;
			std::priority_queue<std::pair<float, size_t>> worst; // (error, chunk): the largest first
			for (size_t c = 0; c < shapes.size(); c++)
			{
				ChunkShape& shape = shapes[c];
				shape.Base = 0;
				// The levels that fit a chunk are the first ones (a coarser step fits only if a finer does); one above a
				// guide's ceiling isn't one: the finer levels a road needs are taken whatever the budget
				for (uint32_t level = 1; level < TerrainLods::LevelCount && shape.Fits[level] && shape.Error[level] != Forbidden; level++)
				{
					shape.Base = level;
				}
				total += vertexCost(shape, shape.Base);
				if (shape.Base > 0 && shape.Error[shape.Base] > ExactError)
				{
					worst.push({ shape.Error[shape.Base], c });
				}
			}
			while (!worst.empty())
			{
				const size_t c = worst.top().second;
				worst.pop();
				ChunkShape& shape = shapes[c];
				const size_t extra = vertexCost(shape, shape.Base - 1) - vertexCost(shape, shape.Base);
				if (total + extra > maxVertices)
				{
					continue; // too many for this one; a chunk with a cheaper step may still fit
				}
				total += extra;
				shape.Base--;
				if (shape.Base > 0 && shape.Error[shape.Base] > ExactError)
				{
					worst.push({ shape.Error[shape.Base], c });
				}
			}
		}

		// Sizes: each chunk's vertices (its base level's grid, then the skirt's lowered copies of its edges) and its own
		// triangles; after all of them, the coarser levels' triangles and every level's skirt. A level finer than the base
		// is the base (its triangles are exact)
		data.Meshes.resize((size_t)chunks * chunks);
		if (lods)
		{
			lods->Chunks.assign((size_t)chunks * chunks, TerrainLods::Chunk());
		}
		size_t vertexTotal = 0, triangleTotal = 0;
		for (uint32_t cz = 0; cz < chunks; cz++)
		{
			for (uint32_t cx = 0; cx < chunks; cx++)
			{
				const ChunkShape& shape = shapes[(size_t)cz * chunks + cx];
				const uint32_t step = 1u << shape.Base, gw = shape.W / step, gd = shape.D / step;
				ModelMeshData& mesh = data.Meshes[(size_t)cz * chunks + cx];
				mesh.Name = "Chunk " + std::to_string(cx) + ", " + std::to_string(cz);
				mesh.BaseVertex = (uint32_t)vertexTotal;
				mesh.BaseIndex = (uint32_t)(triangleTotal * 3);
				mesh.VertexCount = (uint32_t)vertexCost(shape, shape.Base);
				mesh.TriangleCount = gw * gd * 2;
				mesh.IndexCount = mesh.TriangleCount * 3;
				vertexTotal += mesh.VertexCount;
				triangleTotal += mesh.TriangleCount;
			}
		}
		if (lods)
		{
			for (size_t c = 0; c < data.Meshes.size(); c++)
			{
				const ChunkShape& shape = shapes[c];
				TerrainLods::Chunk& chunk = lods->Chunks[c];
				for (uint32_t level = 0; level < TerrainLods::LevelCount; level++)
				{
					chunk.Error[level] = shape.Error[std::max(level, shape.Base)]; // a finer level than the base is the base
				}
				chunk.Surface[shape.Base] = { data.Meshes[c].BaseIndex, data.Meshes[c].IndexCount };
				for (uint32_t level = shape.Base; level < TerrainLods::LevelCount; level++)
				{
					if (level > shape.Base && !shape.Fits[level])
					{
						chunk.Surface[level] = chunk.Surface[level - 1];
						chunk.Skirt[level] = chunk.Skirt[level - 1];
						continue;
					}
					const uint32_t step = 1u << level;
					if (level > shape.Base)
					{
						const uint32_t triangles = (shape.W / step) * (shape.D / step) * 2;
						chunk.Surface[level] = { (uint32_t)(triangleTotal * 3), triangles * 3 };
						triangleTotal += triangles;
					}
					const uint32_t skirtTriangles = (2 * (shape.W / step) + 2 * (shape.D / step)) * 4; // a quad per edge segment, both sides
					chunk.Skirt[level] = { (uint32_t)(triangleTotal * 3), skirtTriangles * 3 };
					triangleTotal += skirtTriangles;
				}
				for (uint32_t level = 0; level < shape.Base; level++)
				{
					chunk.Surface[level] = chunk.Surface[shape.Base];
					chunk.Skirt[level] = chunk.Skirt[shape.Base];
				}
			}
		}
		data.StaticVertices.resize(vertexTotal);
		data.Indices.resize(triangleTotal);

		ParallelRows(chunks, [&](uint32_t first, uint32_t last)
		{
			for (uint32_t cz = first; cz < last; cz++)
			{
				for (uint32_t cx = 0; cx < chunks; cx++)
				{
					const ChunkShape& shape = shapes[(size_t)cz * chunks + cx];
					ModelMeshData& mesh = data.Meshes[(size_t)cz * chunks + cx];
					const uint32_t x0 = cx * cellsPerChunk, z0 = cz * cellsPerChunk;
					const uint32_t w = shape.W, d = shape.D;
					const uint32_t base = 1u << shape.Base, gw = w / base, gd = d / base;
					// A vertex of the base grid, by its cell coordinates in the chunk (multiples of the base level's step)
					auto gridIndex = [&](uint32_t x, uint32_t z) { return (z / base) * (gw + 1) + x / base; };
					float minY = std::numeric_limits<float>::max(), maxY = -std::numeric_limits<float>::max();
					for (uint32_t z = 0; z <= d; z++)
					{
						for (uint32_t x = 0; x <= w; x++)
						{
							const float y = at((int)(x0 + x), (int)(z0 + z));
							minY = std::min(minY, y);
							maxY = std::max(maxY, y);
						}
					}
					for (uint32_t z = 0; z <= d; z += base)
					{
						for (uint32_t x = 0; x <= w; x += base)
						{
							const int gx = (int)(x0 + x), gz = (int)(z0 + z);
							const float y = at(gx, gz);
							const float dx = at(gx + 1, gz) - at(gx - 1, gz), dz = at(gx, gz + 1) - at(gx, gz - 1);
							H2M::VertexH2M& vertex = data.StaticVertices[mesh.BaseVertex + gridIndex(x, z)];
							vertex.Position = glm::vec3(-half + gx * spacing, y, -half + gz * spacing);
							vertex.Normal = glm::normalize(glm::vec3(-dx, 2.0f * spacing, -dz));
							vertex.Tangent = glm::normalize(glm::vec3(2.0f * spacing, dx, 0.0f));
							vertex.Binormal = glm::normalize(glm::vec3(0.0f, dz, 2.0f * spacing));
							vertex.Texcoord = glm::vec2(vertex.Position.x, vertex.Position.z);
						}
					}
					H2M::Index* index = &data.Indices[mesh.BaseIndex / 3];
					for (uint32_t z = 0; z < d; z += base)
					{
						for (uint32_t x = 0; x < w; x += base)
						{
							const uint32_t a = gridIndex(x, z), b = gridIndex(x + base, z), c = gridIndex(x, z + base), e = gridIndex(x + base, z + base);
							*index++ = { a, c, e }; // facing up, split from a to e (-x -z to +x +z)
							*index++ = { a, e, b };
						}
					}
					mesh.BoundsMin = glm::vec3(-half + x0 * spacing, minY, -half + z0 * spacing);
					mesh.BoundsMax = glm::vec3(-half + (x0 + w) * spacing, maxY, -half + (z0 + d) * spacing);
					if (!lods)
					{
						continue;
					}

					// The skirt's vertices: copies of the edges' (a row at z = 0, one at z = d, a column at x = 0, one at
					// x = w), lowered by the chunk's height range and a meter (no coarser level's edge strays further from
					// the heights), with the edges' normals so they shade as the ground beside them
					const float depth = std::max(maxY - minY, 1.0f) + 1.0f;
					const uint32_t grid = (gw + 1) * (gd + 1);
					// The skirt vertex below the grid's (x, z) on an edge (0: z = 0, 1: z = d, 2: x = 0, 3: x = w)
					auto skirtIndex = [&](uint32_t edge, uint32_t x, uint32_t z)
					{
						switch (edge)
						{
							case 0: return grid + x / base;
							case 1: return grid + (gw + 1) + x / base;
							case 2: return grid + 2 * (gw + 1) + z / base;
							default: return grid + 2 * (gw + 1) + (gd + 1) + z / base;
						}
					};
					auto edgePoint = [&](uint32_t edge, uint32_t i, uint32_t& x, uint32_t& z)
					{
						x = edge < 2 ? i : (edge == 2 ? 0 : w);
						z = edge < 2 ? (edge == 0 ? 0 : d) : i;
					};
					for (uint32_t edge = 0; edge < 4; edge++)
					{
						const uint32_t n = edge < 2 ? w : d;
						for (uint32_t i = 0; i <= n; i += base)
						{
							uint32_t x, z;
							edgePoint(edge, i, x, z);
							H2M::VertexH2M vertex = data.StaticVertices[mesh.BaseVertex + gridIndex(x, z)];
							vertex.Position.y -= depth;
							data.StaticVertices[mesh.BaseVertex + skirtIndex(edge, x, z)] = vertex;
						}
					}
					mesh.BoundsMin.y -= depth;

					const TerrainLods::Chunk& chunk = lods->Chunks[(size_t)cz * chunks + cx]; // this chunk's only: no other thread writes it
					for (uint32_t level = shape.Base; level < TerrainLods::LevelCount; level++)
					{
						if (level > shape.Base && !shape.Fits[level])
						{
							continue;
						}
						const uint32_t step = 1u << level;
						if (level > shape.Base)
						{
							H2M::Index* surface = &data.Indices[chunk.Surface[level].BaseIndex / 3];
							for (uint32_t z = 0; z < d; z += step)
							{
								for (uint32_t x = 0; x < w; x += step)
								{
									const uint32_t a = gridIndex(x, z), b = gridIndex(x + step, z), c = gridIndex(x, z + step), e = gridIndex(x + step, z + step);
									*surface++ = { a, c, e };
									*surface++ = { a, e, b };
								}
							}
						}
						H2M::Index* skirt = &data.Indices[chunk.Skirt[level].BaseIndex / 3];
						for (uint32_t edge = 0; edge < 4; edge++)
						{
							const uint32_t n = edge < 2 ? w : d;
							for (uint32_t i = 0; i < n; i += step)
							{
								uint32_t xa, za, xb, zb;
								edgePoint(edge, i, xa, za);
								edgePoint(edge, i + step, xb, zb);
								const uint32_t t0 = gridIndex(xa, za), t1 = gridIndex(xb, zb);
								const uint32_t b0 = skirtIndex(edge, xa, za), b1 = skirtIndex(edge, xb, zb);
								*skirt++ = { t0, b0, b1 }; // one side
								*skirt++ = { t0, b1, t1 };
								*skirt++ = { t0, b1, b0 }; // the other
								*skirt++ = { t0, t1, b1 };
							}
						}
					}
				}
			}
		});

		ModelMaterialData material;
		material.Name = "Terrain";
		material.HasDiffuseColor = true;
		material.DiffuseColor = glm::vec3(0.3f, 0.45f, 0.2f);
		data.Materials.push_back(material);

		AnimationSkeleton::SourceNode root;
		root.Name = "Terrain";
		data.Nodes.push_back(root);
		data.NodeMeshes.resize(1);
		for (uint32_t m = 0; m < (uint32_t)data.Meshes.size(); m++)
		{
			data.NodeMeshes[0].push_back(m);
		}
		return data;
	}

	uint32_t RandomSeed()
	{
		std::random_device device;
		uint32_t seed = 0;
		while (seed == 0)
		{
			seed = device();
		}
		return seed;
	}
}
