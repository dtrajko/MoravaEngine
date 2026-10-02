#include "EnvMapVulkanLights.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>


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

	// Every pixel, not a sampled grid: a sun disc can be only a few pixels wide
	float maxLuminance = 0.0f;
	for (size_t i = 0; i < (size_t)width * height; i++)
	{
		maxLuminance = std::max(maxLuminance, luminance(&rgba[i * 4]));
	}
	if (maxLuminance <= 0.0f)
	{
		return false;
	}

	// Pixel (x, y) -> direction, as in EquirectangularToCubeMap.glsl: u = phi / 2pi + 0.5, v = theta / pi
	const float pi = glm::pi<float>();
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
			if (l < threshold)
			{
				continue;
			}
			const float phi = ((x + 0.5f) / width - 0.5f) * 2.0f * pi;
			const double weight = (double)l * sinTheta; // pixels near the poles cover less of the sphere
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

EnvMapVulkanPointLight* EnvMapVulkanLightEnvironment::AddPointLight(const glm::vec3& position)
{
	if (!CanAddPointLight())
	{
		return nullptr;
	}
	EnvMapVulkanPointLight& light = PointLights.emplace_back();
	light.Name = "Point Light " + std::to_string(m_NextPointLightNumber++);
	light.Position = position;
	return &light;
}

EnvMapVulkanSpotLight* EnvMapVulkanLightEnvironment::AddSpotLight(const glm::vec3& position)
{
	if (!CanAddSpotLight())
	{
		return nullptr;
	}
	EnvMapVulkanSpotLight& light = SpotLights.emplace_back();
	light.Name = "Spot Light " + std::to_string(m_NextSpotLightNumber++);
	light.Position = position;
	return &light;
}

void EnvMapVulkanLightEnvironment::Pack(EnvMapVulkanLightsGPU::LightsUB& out) const
{
	using namespace EnvMapVulkanLightsGPU;

	std::memset(&out, 0, sizeof(LightsUB));

	out.Sun.Direction = Sun.GetDirection();
	out.Sun.Intensity = Sun.Enabled ? std::max(Sun.Intensity, 0.0f) : 0.0f;
	out.Sun.Color = Sun.Color;

	const float minRange = 0.01f; // the falloff window divides by the range

	uint32_t pointCount = 0;
	for (const EnvMapVulkanPointLight& light : PointLights)
	{
		if (!light.Enabled || pointCount == MaxPointLights)
		{
			continue;
		}
		PointLight& gpu = out.PointLights[pointCount++];
		gpu.Position = light.Position;
		gpu.Intensity = std::max(light.Intensity, 0.0f);
		gpu.Color = light.Color;
		gpu.Range = std::max(light.Range, minRange);
	}
	out.PointLightCount = (int32_t)pointCount;

	uint32_t spotCount = 0;
	for (const EnvMapVulkanSpotLight& light : SpotLights)
	{
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
	}
	out.SpotLightCount = (int32_t)spotCount;
}
