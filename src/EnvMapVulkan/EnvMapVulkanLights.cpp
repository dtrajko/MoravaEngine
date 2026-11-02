#include "EnvMapVulkanLights.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>


glm::vec3 DirectionFromAngles(float azimuth, float elevation)
{
	float az = glm::radians(azimuth);
	float el = glm::radians(elevation);
	return glm::vec3(std::cos(el) * std::sin(az), std::sin(el), std::cos(el) * std::cos(az));
}

void AnglesFromDirection(const glm::vec3& direction, float& azimuth, float& elevation)
{
	float length = glm::length(direction);
	if (length < 1e-6f)
	{
		return;
	}
	glm::vec3 d = direction / length;
	elevation = glm::degrees(std::asin(std::clamp(d.y, -1.0f, 1.0f)));
	// Straight up or down the azimuth is undefined: keep the current one
	if (std::abs(d.x) > 1e-6f || std::abs(d.z) > 1e-6f)
	{
		azimuth = glm::degrees(std::atan2(d.x, d.z));
	}
}

bool FindBrightestDirection(const float* rgba, uint32_t width, uint32_t height, glm::vec3& direction, glm::vec3& color)
{
	if (!rgba || width == 0 || height == 0)
	{
		return false;
	}

	auto luminance = [](const float* p) { return 0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2]; };
	const float pi = glm::pi<float>();

	// 1. The light of each cell of a coarse grid (64 x 32 cells, about 5.6 degrees each), every pixel weighted by the
	//    part of the sphere it covers (less near the poles). Every pixel counts: a sun disc can be a few pixels wide.
	const int gridWidth = 64, gridHeight = 32;
	std::vector<double> cells((size_t)gridWidth * gridHeight, 0.0);
	for (uint32_t y = 0; y < height; y++)
	{
		const double solidAngle = std::sin(pi * (y + 0.5f) / height);
		const int cellY = (int)((uint64_t)y * gridHeight / height);
		for (uint32_t x = 0; x < width; x++)
		{
			cells[(size_t)cellY * gridWidth + (size_t)x * gridWidth / width] += luminance(&rgba[((size_t)y * width + x) * 4]) * solidAngle;
		}
	}

	// 2. The brightest block of 3 x 3 cells (the map wraps around horizontally), above the horizon: a sun is never below
	//    it, while sunlit floors and other reflections are. Not the brightest pixel overall, and not the average of all
	//    bright pixels (in a map without a sun they are scattered over several windows and patches of light).
	int bestX = 0, bestY = 0;
	double best = 0.0;
	for (int rows : { gridHeight / 2, gridHeight }) // above the horizon; the whole map only if the sky is black
	{
		for (int cy = 0; cy < rows; cy++)
		{
			for (int cx = 0; cx < gridWidth; cx++)
			{
				double sum = 0.0;
				for (int dy = -1; dy <= 1; dy++)
				{
					if (cy + dy < 0 || cy + dy >= gridHeight)
					{
						continue;
					}
					for (int dx = -1; dx <= 1; dx++)
					{
						sum += cells[(size_t)(cy + dy) * gridWidth + (cx + dx + gridWidth) % gridWidth];
					}
				}
				if (sum > best)
				{
					best = sum;
					bestX = cx;
					bestY = cy;
				}
			}
		}
		if (best > 0.0)
		{
			break;
		}
	}
	if (best <= 0.0)
	{
		return false;
	}

	// 3. In that block: the pixels at least half as bright as its brightest pixel (the sun disc, or a bright window),
	//    their luminance-weighted center and their average color
	auto inBlock = [&](uint32_t x, uint32_t y) {
		int cellY = (int)((uint64_t)y * gridHeight / height);
		int cellX = (int)((uint64_t)x * gridWidth / width);
		return std::abs(cellY - bestY) <= 1 && ((cellX - bestX + gridWidth + 1) % gridWidth) <= 2;
	};
	float maxLuminance = 0.0f;
	for (uint32_t y = 0; y < height; y++)
	{
		for (uint32_t x = 0; x < width; x++)
		{
			if (inBlock(x, y))
			{
				maxLuminance = std::max(maxLuminance, luminance(&rgba[((size_t)y * width + x) * 4]));
			}
		}
	}

	// Pixel (x, y) -> direction, as in EquirectangularToCubeMap.glsl: u = phi / 2pi + 0.5, v = theta / pi
	const float threshold = maxLuminance * 0.5f;
	glm::dvec3 weightedDirection(0.0);
	glm::dvec3 weightedColor(0.0);
	double weightSum = 0.0;
	for (uint32_t y = 0; y < height; y++)
	{
		const float theta = pi * (y + 0.5f) / height;
		const float sinTheta = std::sin(theta);
		const float cosTheta = std::cos(theta);
		for (uint32_t x = 0; x < width; x++)
		{
			const float* p = &rgba[((size_t)y * width + x) * 4];
			const float l = luminance(p);
			if (l < threshold || !inBlock(x, y))
			{
				continue;
			}
			const float phi = ((x + 0.5f) / width - 0.5f) * 2.0f * pi;
			const double weight = (double)l * sinTheta;
			weightedDirection += weight * glm::dvec3(sinTheta * std::cos(phi), cosTheta, sinTheta * std::sin(phi));
			weightedColor += (double)sinTheta * glm::dvec3(p[0], p[1], p[2]);
			weightSum += sinTheta;
		}
	}
	if (glm::length(weightedDirection) < 1e-12 || weightSum <= 0.0)
	{
		return false;
	}

	direction = glm::normalize(glm::vec3(weightedDirection));
	glm::vec3 averageColor = glm::vec3(weightedColor / weightSum);
	float maxChannel = std::max(averageColor.r, std::max(averageColor.g, averageColor.b));
	color = maxChannel > 0.0f ? averageColor / maxChannel : glm::vec3(1.0f);
	return true;
}

// Pixel (x, y) of an equirectangular map -> unit direction, as in EquirectangularToCubeMap.glsl
static glm::vec3 EquirectangularDirection(float x, float y, uint32_t width, uint32_t height)
{
	const float pi = glm::pi<float>();
	const float theta = pi * (y + 0.5f) / height;
	const float phi = ((x + 0.5f) / width - 0.5f) * 2.0f * pi;
	return glm::vec3(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
}

bool ExtractSun(float* rgba, uint32_t width, uint32_t height, EnvMapVulkanExtractedSun& sun)
{
	sun = EnvMapVulkanExtractedSun();
	if (!rgba || width < 2 || height < 2)
	{
		return false;
	}
	auto luminance = [](const float* p) { return 0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2]; };
	const double pi = glm::pi<double>();
	const double pixelSolidAngle = (2.0 * pi / width) * (pi / height); // times sin(theta) of the pixel's row

	// The map's average (over the sphere) and its brightest pixel above the horizon
	double sum = 0.0, area = 0.0;
	float peak = 0.0f;
	uint32_t peakX = 0, peakY = 0;
	for (uint32_t y = 0; y < height; y++)
	{
		const double sinTheta = std::sin(pi * (y + 0.5) / height);
		for (uint32_t x = 0; x < width; x++)
		{
			const float l = luminance(&rgba[((size_t)y * width + x) * 4]);
			sum += l * sinTheta;
			area += sinTheta;
			if (y < height / 2 && l > peak)
			{
				peak = l;
				peakX = x;
				peakY = y;
			}
		}
	}
	const double average = sum / area;
	if (average <= 0.0 || peak < 1000.0 * average)
	{
		return false; // no sun: nothing in the sky stands out that much
	}

	// The disc and its glow: pixels within 5 degrees of the peak, brighter than 50x the average
	const float threshold = (float)(50.0 * average);
	const glm::vec3 center = EquirectangularDirection((float)peakX, (float)peakY, width, height);
	const float cosRadius = std::cos(glm::radians(5.0f));
	const int rowRadius = (int)std::ceil(5.0 / 180.0 * height) + 1;
	const int firstRow = std::max((int)peakY - rowRadius, 0);
	const int lastRow = std::min((int)peakY + rowRadius, (int)height - 1);

	glm::dvec3 removed(0.0);           // irradiance taken out (rgb)
	glm::dvec3 weightedDirection(0.0);
	for (int y = firstRow; y <= lastRow; y++)
	{
		const double solidAngle = pixelSolidAngle * std::sin(pi * (y + 0.5) / height);
		for (uint32_t x = 0; x < width; x++)
		{
			float* p = &rgba[((size_t)y * width + x) * 4];
			const float l = luminance(p);
			if (l <= threshold)
			{
				continue;
			}
			const glm::vec3 direction = EquirectangularDirection((float)x, (float)y, width, height);
			if (glm::dot(direction, center) < cosRadius)
			{
				continue;
			}
			// Dimmed to the threshold (same color); the light above it is moved to the directional light
			const float keep = threshold / l;
			const glm::dvec3 taken = glm::dvec3(p[0], p[1], p[2]) * (1.0 - keep) * solidAngle;
			removed += taken;
			weightedDirection += glm::dvec3(direction) * (0.2126 * taken.r + 0.7152 * taken.g + 0.0722 * taken.b);
			p[0] *= keep;
			p[1] *= keep;
			p[2] *= keep;
			sun.PixelCount++;
		}
	}
	const double maxChannel = std::max(removed.r, std::max(removed.g, removed.b));
	if (sun.PixelCount == 0 || maxChannel <= 0.0)
	{
		return false;
	}

	sun.Found = true;
	sun.Direction = glm::normalize(glm::vec3(weightedDirection));
	sun.Color = glm::vec3(removed / maxChannel);
	sun.Intensity = (float)(maxChannel / pi);
	sun.PeakToAverage = (float)(peak / average);
	return true;
}

EnvMapVulkanPointLight* EnvMapVulkanLightEnvironment::AddPointLight(const glm::vec3& position, uint32_t maxShadowed)
{
	if (!CanAddPointLight())
	{
		return nullptr;
	}
	const bool castShadows = CountShadowedPointLights() < maxShadowed;
	EnvMapVulkanPointLight& light = PointLights.emplace_back();
	light.Name = "Point Light " + std::to_string(m_NextPointLightNumber++);
	light.Position = position;
	light.CastShadows = castShadows;
	return &light;
}

EnvMapVulkanSpotLight* EnvMapVulkanLightEnvironment::AddSpotLight(const glm::vec3& position, uint32_t maxShadowed)
{
	if (!CanAddSpotLight())
	{
		return nullptr;
	}
	const bool castShadows = CountShadowedSpotLights() < maxShadowed;
	EnvMapVulkanSpotLight& light = SpotLights.emplace_back();
	light.Name = "Spot Light " + std::to_string(m_NextSpotLightNumber++);
	light.Position = position;
	light.CastShadows = castShadows;
	return &light;
}

uint32_t EnvMapVulkanLightEnvironment::CountShadowedPointLights() const
{
	return (uint32_t)std::count_if(PointLights.begin(), PointLights.end(), [](const EnvMapVulkanPointLight& light) { return light.CastShadows; });
}

uint32_t EnvMapVulkanLightEnvironment::CountShadowedSpotLights() const
{
	return (uint32_t)std::count_if(SpotLights.begin(), SpotLights.end(), [](const EnvMapVulkanSpotLight& light) { return light.CastShadows; });
}

void EnvMapVulkanLightEnvironment::Pack(EnvMapVulkanLightsGPU::LightsUB& out, const std::vector<int>& pointShadowSlots,
	const std::vector<int>& spotShadowSlots) const
{
	using namespace EnvMapVulkanLightsGPU;

	std::memset(&out, 0, sizeof(LightsUB));

	out.Sun.Direction = Sun.GetDirection();
	out.Sun.Intensity = Sun.Enabled ? std::max(Sun.Intensity, 0.0f) : 0.0f;
	out.Sun.Color = Sun.Color;

	const float minRange = 0.01f; // the falloff window divides by the range

	uint32_t pointCount = 0;
	for (size_t i = 0; i < PointLights.size(); i++)
	{
		const EnvMapVulkanPointLight& light = PointLights[i];
		if (!light.Enabled || pointCount == MaxPointLights)
		{
			continue;
		}
		PointLight& gpu = out.PointLights[pointCount++];
		gpu.Position = light.Position;
		gpu.Intensity = std::max(light.Intensity, 0.0f);
		gpu.Color = light.Color;
		gpu.Range = std::max(light.Range, minRange);
		gpu.ShadowIndex = i < pointShadowSlots.size() ? (float)pointShadowSlots[i] : -1.0f;
	}
	out.PointLightCount = (int32_t)pointCount;

	uint32_t spotCount = 0;
	for (size_t i = 0; i < SpotLights.size(); i++)
	{
		const EnvMapVulkanSpotLight& light = SpotLights[i];
		if (!light.Enabled || spotCount == MaxSpotLights)
		{
			continue;
		}
		// The outer angle stays below 90 degrees (a cone, not a hemisphere) and the inner angle inside it,
		// slightly smaller so the smoothstep between them never divides by zero
		float outer = std::clamp(light.OuterAngle, 0.1f, 89.0f);
		float inner = std::clamp(light.InnerAngle, 0.0f, outer - 0.05f);

		SpotLight& gpu = out.SpotLights[spotCount++];
		gpu.Position = light.Position;
		gpu.Intensity = std::max(light.Intensity, 0.0f);
		gpu.Color = light.Color;
		gpu.Range = std::max(light.Range, minRange);
		gpu.Direction = light.GetDirection();
		gpu.CosOuter = std::cos(glm::radians(outer));
		gpu.CosInner = std::cos(glm::radians(inner));
		gpu.ShadowIndex = i < spotShadowSlots.size() ? (float)spotShadowSlots[i] : -1.0f;
	}
	out.SpotLightCount = (int32_t)spotCount;
}
