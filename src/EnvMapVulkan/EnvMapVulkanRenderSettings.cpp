#include "EnvMapVulkanRenderSettings.h"


template<typename T>
static void ReadSetting(const YAML::Node& node, const char* key, T& value)
{
	if (node && node[key])
	{
		value = node[key].as<T>();
	}
}

void EnvMapVulkanRenderSettings::Serialize(YAML::Emitter& out) const
{
	out << YAML::Key << "Shadows" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Distance" << YAML::Value << Shadows.Distance;
	out << YAML::Key << "Resolution" << YAML::Value << Shadows.Resolution;
	out << YAML::Key << "SplitLambda" << YAML::Value << Shadows.SplitLambda;
	out << YAML::Key << "Softness" << YAML::Value << Shadows.Softness;
	out << YAML::Key << "DepthBias" << YAML::Value << Shadows.DepthBias;
	out << YAML::Key << "SlopeBias" << YAML::Value << Shadows.SlopeBias;
	out << YAML::Key << "NormalBias" << YAML::Value << Shadows.NormalBias;
	out << YAML::EndMap;

	out << YAML::Key << "LocalShadows" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "SpotResolution" << YAML::Value << LocalShadows.SpotResolution;
	out << YAML::Key << "PointResolution" << YAML::Value << LocalShadows.PointResolution;
	out << YAML::Key << "Softness" << YAML::Value << LocalShadows.Softness;
	out << YAML::Key << "DepthBias" << YAML::Value << LocalShadows.DepthBias;
	out << YAML::Key << "SlopeBias" << YAML::Value << LocalShadows.SlopeBias;
	out << YAML::Key << "NormalBias" << YAML::Value << LocalShadows.NormalBias;
	out << YAML::EndMap;

	out << YAML::Key << "RayTracing" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Enabled" << YAML::Value << RayTracing;
	out << YAML::EndMap;

	out << YAML::Key << "Reflections" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Enabled" << YAML::Value << Reflections.Enabled;
	out << YAML::Key << "MaxRoughness" << YAML::Value << Reflections.MaxRoughness;
	out << YAML::Key << "Denoise" << YAML::Value << Reflections.Denoise;
	out << YAML::Key << "Temporal" << YAML::Value << Reflections.Temporal;
	out << YAML::Key << "HalfResolution" << YAML::Value << Reflections.HalfResolution;
	out << YAML::EndMap;

	out << YAML::Key << "AmbientOcclusion" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Enabled" << YAML::Value << AmbientOcclusion.Enabled;
	out << YAML::Key << "Radius" << YAML::Value << AmbientOcclusion.Radius;
	out << YAML::Key << "Power" << YAML::Value << AmbientOcclusion.Power;
	out << YAML::Key << "Falloff" << YAML::Value << AmbientOcclusion.Falloff;
	out << YAML::Key << "Slices" << YAML::Value << AmbientOcclusion.Slices;
	out << YAML::Key << "Steps" << YAML::Value << AmbientOcclusion.Steps;
	out << YAML::Key << "Denoise" << YAML::Value << AmbientOcclusion.Denoise;
	out << YAML::Key << "Temporal" << YAML::Value << AmbientOcclusion.Temporal;
	out << YAML::Key << "HalfResolution" << YAML::Value << AmbientOcclusion.HalfResolution;
	out << YAML::Key << "RayTraced" << YAML::Value << AmbientOcclusion.RayTraced;
	out << YAML::Key << "Rays" << YAML::Value << AmbientOcclusion.Rays;
	out << YAML::EndMap;

	out << YAML::Key << "Bloom" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Enabled" << YAML::Value << Bloom.Enabled;
	out << YAML::Key << "Threshold" << YAML::Value << Bloom.Threshold;
	out << YAML::Key << "Knee" << YAML::Value << Bloom.Knee;
	out << YAML::Key << "UpsampleScale" << YAML::Value << Bloom.UpsampleScale;
	out << YAML::Key << "Intensity" << YAML::Value << Bloom.Intensity;
	out << YAML::Key << "DirtEnabled" << YAML::Value << Bloom.DirtEnabled;
	out << YAML::Key << "DirtIntensity" << YAML::Value << Bloom.DirtIntensity;
	out << YAML::EndMap;
}

EnvMapVulkanRenderSettings EnvMapVulkanRenderSettings::Deserialize(const YAML::Node& node)
{
	EnvMapVulkanRenderSettings settings;
	if (!node)
	{
		return settings;
	}
	if (YAML::Node shadows = node["Shadows"])
	{
		EnvMapVulkanShadowSettings& s = settings.Shadows;
		ReadSetting(shadows, "Distance", s.Distance);
		ReadSetting(shadows, "Resolution", s.Resolution);
		ReadSetting(shadows, "SplitLambda", s.SplitLambda);
		ReadSetting(shadows, "Softness", s.Softness);
		ReadSetting(shadows, "DepthBias", s.DepthBias);
		ReadSetting(shadows, "SlopeBias", s.SlopeBias);
		ReadSetting(shadows, "NormalBias", s.NormalBias);
	}
	if (YAML::Node local = node["LocalShadows"])
	{
		EnvMapVulkanLocalShadowSettings& s = settings.LocalShadows;
		ReadSetting(local, "SpotResolution", s.SpotResolution);
		ReadSetting(local, "PointResolution", s.PointResolution);
		ReadSetting(local, "Softness", s.Softness);
		ReadSetting(local, "DepthBias", s.DepthBias);
		ReadSetting(local, "SlopeBias", s.SlopeBias);
		ReadSetting(local, "NormalBias", s.NormalBias);
	}
	if (YAML::Node rayTracing = node["RayTracing"])
	{
		ReadSetting(rayTracing, "Enabled", settings.RayTracing);
	}
	if (YAML::Node reflections = node["Reflections"])
	{
		EnvMapVulkanReflectionSettings& r = settings.Reflections;
		ReadSetting(reflections, "Enabled", r.Enabled);
		ReadSetting(reflections, "MaxRoughness", r.MaxRoughness);
		ReadSetting(reflections, "Denoise", r.Denoise);
		ReadSetting(reflections, "Temporal", r.Temporal);
		ReadSetting(reflections, "HalfResolution", r.HalfResolution);
	}
	if (YAML::Node occlusion = node["AmbientOcclusion"])
	{
		EnvMapVulkanSSAOSettings& o = settings.AmbientOcclusion;
		ReadSetting(occlusion, "Enabled", o.Enabled);
		ReadSetting(occlusion, "Radius", o.Radius);
		ReadSetting(occlusion, "Power", o.Power);
		ReadSetting(occlusion, "Falloff", o.Falloff);
		ReadSetting(occlusion, "Slices", o.Slices);
		ReadSetting(occlusion, "Steps", o.Steps);
		ReadSetting(occlusion, "Denoise", o.Denoise);
		ReadSetting(occlusion, "Temporal", o.Temporal);
		ReadSetting(occlusion, "HalfResolution", o.HalfResolution);
		ReadSetting(occlusion, "RayTraced", o.RayTraced);
		ReadSetting(occlusion, "Rays", o.Rays);
	}
	if (YAML::Node bloom = node["Bloom"])
	{
		EnvMapVulkanBloomSettings& b = settings.Bloom;
		ReadSetting(bloom, "Enabled", b.Enabled);
		ReadSetting(bloom, "Threshold", b.Threshold);
		ReadSetting(bloom, "Knee", b.Knee);
		ReadSetting(bloom, "UpsampleScale", b.UpsampleScale);
		ReadSetting(bloom, "Intensity", b.Intensity);
		ReadSetting(bloom, "DirtEnabled", b.DirtEnabled);
		ReadSetting(bloom, "DirtIntensity", b.DirtIntensity);
	}
	return settings;
}
