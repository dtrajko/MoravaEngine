#include "EnvMapVulkanSceneSerializer.h"

#include "EnvMapVulkanMaterialLibrary.h"

#include <filesystem>
#include <fstream>


namespace EnvMapVulkanSceneSerializer
{
	std::string ToStoredPath(const std::string& path)
	{
		if (path.empty())
		{
			return path;
		}
		std::error_code error;
		std::filesystem::path absolute = std::filesystem::weakly_canonical(path, error);
		if (error)
		{
			return std::filesystem::path(path).generic_string();
		}
		std::filesystem::path relative = absolute.lexically_relative(std::filesystem::current_path(error));
		std::filesystem::path stored = (!relative.empty() && *relative.begin() != "..") ? relative : absolute;
		return stored.generic_string();
	}

	static void WriteVec3(YAML::Emitter& out, const char* key, const glm::vec3& v)
	{
		out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	}

	static void ReadVec3(const YAML::Node& node, glm::vec3& v)
	{
		if (node && node.IsSequence() && node.size() == 3)
		{
			v = glm::vec3(node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
		}
	}

	template<typename T>
	static void Read(const YAML::Node& node, const char* key, T& value)
	{
		if (node[key])
		{
			value = node[key].as<T>();
		}
	}

	// The water's settings, except its placement (the entity's transform)
	static void WriteWater(YAML::Emitter& out, const EnvMapVulkanWaterSettings& w)
	{
		out << YAML::Key << "Water" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "WaveDirection" << YAML::Value << w.WaveDirection;
		out << YAML::Key << "WaveSpeed" << YAML::Value << w.WaveSpeed;
		out << YAML::Key << "WaveStrength" << YAML::Value << w.WaveStrength;
		out << YAML::Key << "WaveScale1" << YAML::Value << w.WaveScale1;
		out << YAML::Key << "WaveScale2" << YAML::Value << w.WaveScale2;
		out << YAML::Key << "SwellHeight" << YAML::Value << w.SwellHeight;
		out << YAML::Key << "SwellLength" << YAML::Value << w.SwellLength;
		out << YAML::Key << "SwellSteepness" << YAML::Value << w.SwellSteepness;
		out << YAML::Key << "Wireframe" << YAML::Value << w.Wireframe;
		WriteVec3(out, "ScatterColor", w.ScatterColor);
		out << YAML::Key << "Roughness" << YAML::Value << w.Roughness;
		out << YAML::Key << "ReflectionStrength" << YAML::Value << w.ReflectionStrength;
		WriteVec3(out, "Transmittance", w.Transmittance);
		out << YAML::Key << "Clarity" << YAML::Value << w.Clarity;
		out << YAML::Key << "RefractionStrength" << YAML::Value << w.RefractionStrength;
		out << YAML::Key << "EdgeSoftness" << YAML::Value << w.EdgeSoftness;
		out << YAML::Key << "FoamAmount" << YAML::Value << w.FoamAmount;
		out << YAML::Key << "FoamWidth" << YAML::Value << w.FoamWidth;
		out << YAML::Key << "TransparencyFromBelow" << YAML::Value << w.TransparencyFromBelow;
		out << YAML::Key << "PlanarReflection" << YAML::Value << w.PlanarReflection;
		out << YAML::Key << "ReflectionDivisor" << YAML::Value << w.ReflectionDivisor;
		out << YAML::Key << "ReflectionDistortion" << YAML::Value << w.ReflectionDistortion;
		out << YAML::Key << "Caustics" << YAML::Value << w.Caustics;
		out << YAML::Key << "CausticsStrength" << YAML::Value << w.CausticsStrength;
		out << YAML::Key << "CausticsFocus" << YAML::Value << w.CausticsFocus;
		out << YAML::Key << "CausticsArea" << YAML::Value << w.CausticsArea;
		out << YAML::EndMap;
	}

	static void ReadWater(const YAML::Node& node, EnvMapVulkanWaterSettings& w)
	{
		Read(node, "WaveDirection", w.WaveDirection);
		Read(node, "WaveSpeed", w.WaveSpeed);
		Read(node, "WaveStrength", w.WaveStrength);
		Read(node, "WaveScale1", w.WaveScale1);
		Read(node, "WaveScale2", w.WaveScale2);
		Read(node, "SwellHeight", w.SwellHeight);
		Read(node, "SwellLength", w.SwellLength);
		Read(node, "SwellSteepness", w.SwellSteepness);
		Read(node, "Wireframe", w.Wireframe);
		ReadVec3(node["ScatterColor"], w.ScatterColor);
		Read(node, "Roughness", w.Roughness);
		Read(node, "ReflectionStrength", w.ReflectionStrength);
		ReadVec3(node["Transmittance"], w.Transmittance);
		Read(node, "Clarity", w.Clarity);
		Read(node, "RefractionStrength", w.RefractionStrength);
		Read(node, "EdgeSoftness", w.EdgeSoftness);
		Read(node, "FoamAmount", w.FoamAmount);
		Read(node, "FoamWidth", w.FoamWidth);
		Read(node, "TransparencyFromBelow", w.TransparencyFromBelow);
		Read(node, "PlanarReflection", w.PlanarReflection);
		Read(node, "ReflectionDivisor", w.ReflectionDivisor);
		Read(node, "ReflectionDistortion", w.ReflectionDistortion);
		Read(node, "Caustics", w.Caustics);
		Read(node, "CausticsStrength", w.CausticsStrength);
		Read(node, "CausticsFocus", w.CausticsFocus);
		Read(node, "CausticsArea", w.CausticsArea);
	}

	static void WriteEntity(YAML::Emitter& out, EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "ID" << YAML::Value << entity;
		out << YAML::Key << "Name" << YAML::Value << scene.Get<NameComponent>(entity).Name;
		out << YAML::Key << "Parent" << YAML::Value << scene.GetParent(entity);

		const TransformComponent& transform = scene.Get<TransformComponent>(entity);
		out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
		WriteVec3(out, "Translation", transform.Translation);
		WriteVec3(out, "Rotation", transform.Rotation);
		WriteVec3(out, "Scale", transform.Scale);
		out << YAML::EndMap;

		if (const ModelComponent* model = scene.TryGet<ModelComponent>(entity))
		{
			out << YAML::Key << "Model" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "File" << YAML::Value << ToStoredPath(model->FilePath);
			out << YAML::EndMap;
		}
		if (const MeshPartComponent* part = scene.TryGet<MeshPartComponent>(entity))
		{
			out << YAML::Key << "MeshPart" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "SourceMeshIndex" << YAML::Value << part->SourceMeshIndex;
			if (part->Material)
			{
				out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;
				out << YAML::Key << "ID" << YAML::Value << part->Material->GetID();
				out << YAML::Key << "File" << YAML::Value << part->Material->GetFilePath();
				out << YAML::Key << "Name" << YAML::Value << part->Material->GetName(); // for people reading the file
				out << YAML::EndMap;
			}
			out << YAML::EndMap;
		}
		if (const SunComponent* sun = scene.TryGet<SunComponent>(entity))
		{
			out << YAML::Key << "Sun" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Enabled" << YAML::Value << sun->Enabled;
			WriteVec3(out, "Color", sun->Color);
			out << YAML::Key << "Intensity" << YAML::Value << sun->Intensity;
			out << YAML::Key << "FollowEnvironmentRotation" << YAML::Value << sun->FollowEnvironmentRotation;
			out << YAML::Key << "CastShadows" << YAML::Value << sun->CastShadows;
			out << YAML::Key << "IconPlaced" << YAML::Value << sun->IconPlaced;
			out << YAML::EndMap;
		}
		if (const PointLightComponent* light = scene.TryGet<PointLightComponent>(entity))
		{
			out << YAML::Key << "PointLight" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Enabled" << YAML::Value << light->Enabled;
			WriteVec3(out, "Color", light->Color);
			out << YAML::Key << "Intensity" << YAML::Value << light->Intensity;
			out << YAML::Key << "Range" << YAML::Value << light->Range;
			out << YAML::Key << "CastShadows" << YAML::Value << light->CastShadows;
			out << YAML::EndMap;
		}
		if (const SpotLightComponent* light = scene.TryGet<SpotLightComponent>(entity))
		{
			out << YAML::Key << "SpotLight" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Enabled" << YAML::Value << light->Enabled;
			WriteVec3(out, "Color", light->Color);
			out << YAML::Key << "Intensity" << YAML::Value << light->Intensity;
			out << YAML::Key << "Range" << YAML::Value << light->Range;
			out << YAML::Key << "InnerAngle" << YAML::Value << light->InnerAngle;
			out << YAML::Key << "OuterAngle" << YAML::Value << light->OuterAngle;
			out << YAML::Key << "CastShadows" << YAML::Value << light->CastShadows;
			out << YAML::EndMap;
		}
		if (const WaterComponent* water = scene.TryGet<WaterComponent>(entity))
		{
			WriteWater(out, water->Settings);
		}
		if (const ProbeVolumeComponent* probes = scene.TryGet<ProbeVolumeComponent>(entity))
		{
			// Its placement (center and size) is the entity's transform
			const EnvMapVulkanProbeVolumeSettings& p = probes->Settings;
			out << YAML::Key << "ProbeVolume" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Enabled" << YAML::Value << p.Enabled;
			out << YAML::Key << "Counts" << YAML::Value << YAML::Flow << YAML::BeginSeq << p.Counts.x << p.Counts.y << p.Counts.z << YAML::EndSeq;
			out << YAML::Key << "NormalBias" << YAML::Value << p.NormalBias;
			out << YAML::Key << "ViewBias" << YAML::Value << p.ViewBias;
			out << YAML::Key << "ShowProbes" << YAML::Value << p.ShowProbes;
			out << YAML::Key << "ProbeRadius" << YAML::Value << p.ProbeRadius;
			out << YAML::EndMap;
		}
		if (const EnvironmentComponent* environment = scene.TryGet<EnvironmentComponent>(entity))
		{
			out << YAML::Key << "Environment" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "File" << YAML::Value << ToStoredPath(environment->FilePath);
			out << YAML::Key << "Rotation" << YAML::Value << environment->Rotation;
			out << YAML::Key << "Exposure" << YAML::Value << environment->Exposure;
			out << YAML::Key << "AutoExposure" << YAML::Value << environment->AutoExposure;
			out << YAML::Key << "HuePreservation" << YAML::Value << environment->HuePreservation;
			out << YAML::Key << "ExtractSun" << YAML::Value << environment->ExtractSun;
			out << YAML::Key << "SkyboxLod" << YAML::Value << environment->SkyboxLod;
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		for (EnvMapVulkanEntityID child : scene.GetChildren(entity))
		{
			WriteEntity(out, scene, child);
		}
	}

	std::string Serialize(EnvMapVulkanScene& scene, const std::string& sceneName, const std::function<void(YAML::Emitter&)>& writeRenderSettings)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "Version" << YAML::Value << 1;
		out << YAML::Key << "Name" << YAML::Value << sceneName;
		out << YAML::EndMap;
		if (writeRenderSettings)
		{
			out << YAML::Key << "RenderSettings" << YAML::Value << YAML::BeginMap;
			writeRenderSettings(out);
			out << YAML::EndMap;
		}
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (EnvMapVulkanEntityID root : scene.GetRoots())
		{
			WriteEntity(out, scene, root);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
		return std::string(out.c_str()) + "\n";
	}

	bool Deserialize(const std::string& filepath, EnvMapVulkanScene& scene, std::string& sceneName, YAML::Node& renderSettings,
		std::unordered_map<EnvMapVulkanEntityID, MaterialRef>& materialRefs, std::string& error)
	{
		YAML::Node root;
		try
		{
			root = YAML::LoadFile(filepath);
		}
		catch (const std::exception& e)
		{
			error = e.what();
			return false;
		}
		if (!root["Scene"] || !root["Entities"] || !root["Entities"].IsSequence())
		{
			error = "not a scene file (no Scene and Entities)";
			return false;
		}
		sceneName = root["Scene"]["Name"].as<std::string>(std::filesystem::path(filepath).stem().string());
		renderSettings = root["RenderSettings"];

		try
		{
			for (const YAML::Node& node : root["Entities"])
			{
				const EnvMapVulkanEntityID id = node["ID"].as<uint64_t>(NoEntity);
				const EnvMapVulkanEntityID parent = node["Parent"].as<uint64_t>(NoEntity);
				const EnvMapVulkanEntityID entity = scene.CreateEntity(node["Name"].as<std::string>("Entity"), parent, id);

				if (YAML::Node transformNode = node["Transform"])
				{
					TransformComponent& transform = scene.Get<TransformComponent>(entity);
					ReadVec3(transformNode["Translation"], transform.Translation);
					ReadVec3(transformNode["Rotation"], transform.Rotation);
					ReadVec3(transformNode["Scale"], transform.Scale);
				}
				if (YAML::Node modelNode = node["Model"])
				{
					scene.Add<ModelComponent>(entity).FilePath = modelNode["File"].as<std::string>("");
				}
				if (YAML::Node partNode = node["MeshPart"])
				{
					MeshPartComponent& part = scene.Add<MeshPartComponent>(entity);
					part.SourceMeshIndex = partNode["SourceMeshIndex"].as<uint32_t>(0);
					part.MeshIndex = part.SourceMeshIndex; // the caller sets it once the model's meshes are known
					if (YAML::Node materialNode = partNode["Material"])
					{
						materialRefs[entity] = { materialNode["ID"].as<uint64_t>(0), materialNode["File"].as<std::string>("") };
					}
				}
				if (YAML::Node sunNode = node["Sun"])
				{
					SunComponent& sun = scene.Add<SunComponent>(entity);
					Read(sunNode, "Enabled", sun.Enabled);
					ReadVec3(sunNode["Color"], sun.Color);
					Read(sunNode, "Intensity", sun.Intensity);
					Read(sunNode, "FollowEnvironmentRotation", sun.FollowEnvironmentRotation);
					Read(sunNode, "CastShadows", sun.CastShadows);
					Read(sunNode, "IconPlaced", sun.IconPlaced);
				}
				if (YAML::Node lightNode = node["PointLight"])
				{
					PointLightComponent& light = scene.Add<PointLightComponent>(entity);
					Read(lightNode, "Enabled", light.Enabled);
					ReadVec3(lightNode["Color"], light.Color);
					Read(lightNode, "Intensity", light.Intensity);
					Read(lightNode, "Range", light.Range);
					Read(lightNode, "CastShadows", light.CastShadows);
				}
				if (YAML::Node lightNode = node["SpotLight"])
				{
					SpotLightComponent& light = scene.Add<SpotLightComponent>(entity);
					Read(lightNode, "Enabled", light.Enabled);
					ReadVec3(lightNode["Color"], light.Color);
					Read(lightNode, "Intensity", light.Intensity);
					Read(lightNode, "Range", light.Range);
					Read(lightNode, "InnerAngle", light.InnerAngle);
					Read(lightNode, "OuterAngle", light.OuterAngle);
					Read(lightNode, "CastShadows", light.CastShadows);
				}
				if (YAML::Node waterNode = node["Water"])
				{
					WaterComponent& water = scene.Add<WaterComponent>(entity);
					ReadWater(waterNode, water.Settings);
				}
				if (YAML::Node probesNode = node["ProbeVolume"])
				{
					EnvMapVulkanProbeVolumeSettings& p = scene.Add<ProbeVolumeComponent>(entity).Settings;
					Read(probesNode, "Enabled", p.Enabled);
					if (YAML::Node counts = probesNode["Counts"]; counts && counts.IsSequence() && counts.size() == 3)
					{
						p.Counts = glm::ivec3(counts[0].as<int>(), counts[1].as<int>(), counts[2].as<int>());
					}
					Read(probesNode, "NormalBias", p.NormalBias);
					Read(probesNode, "ViewBias", p.ViewBias);
					Read(probesNode, "ShowProbes", p.ShowProbes);
					Read(probesNode, "ProbeRadius", p.ProbeRadius);
				}
				if (YAML::Node environmentNode = node["Environment"])
				{
					EnvironmentComponent& environment = scene.Add<EnvironmentComponent>(entity);
					Read(environmentNode, "File", environment.FilePath);
					Read(environmentNode, "Rotation", environment.Rotation);
					Read(environmentNode, "Exposure", environment.Exposure);
					Read(environmentNode, "AutoExposure", environment.AutoExposure);
					Read(environmentNode, "HuePreservation", environment.HuePreservation);
					Read(environmentNode, "ExtractSun", environment.ExtractSun);
					Read(environmentNode, "SkyboxLod", environment.SkyboxLod);
				}
			}
		}
		catch (const std::exception& e)
		{
			error = std::string("an entity could not be read: ") + e.what();
			return false;
		}
		return true;
	}
}
