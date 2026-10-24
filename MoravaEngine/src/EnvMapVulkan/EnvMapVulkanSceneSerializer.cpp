#include "EnvMapVulkanSceneSerializer.h"

#include "EnvMapVulkanMaterialLibrary.h"

#include "Project/Project.h"

#include <filesystem>
#include <fstream>


namespace EnvMapVulkanSceneSerializer
{
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

	// A script's fields, each with its type, so they are read back as they were without the game's assembly:
	//   Fields:
	//     Speed: {Float: 4}
	//     Camera: {Entity: 4797481855757227527}
	//     Around: {Enum: Y}
	static void WriteScript(YAML::Emitter& out, const ScriptComponent& script)
	{
		out << YAML::Key << "Script" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "Class" << YAML::Value << script.ClassName;
		if (!script.Fields.empty())
		{
			out << YAML::Key << "Fields" << YAML::Value << YAML::BeginMap;
			for (const ScriptComponent::Field& field : script.Fields)
			{
				const ScriptFieldValue& v = field.Value;
				out << YAML::Key << field.Name << YAML::Value << YAML::Flow << YAML::BeginMap << YAML::Key << ScriptFieldValue::GetTypeName(v.Type) << YAML::Value;
				switch (v.Type)
				{
				case ScriptFieldType::Bool:    out << (v.Integer != 0); break;
				case ScriptFieldType::Int:     out << v.Integer; break;
				case ScriptFieldType::Float:   out << float(v.Number); break;
				case ScriptFieldType::Double:  out << v.Number; break;
				case ScriptFieldType::Vector2: out << YAML::Flow << YAML::BeginSeq << v.Vector.x << v.Vector.y << YAML::EndSeq; break;
				case ScriptFieldType::Vector3: out << YAML::Flow << YAML::BeginSeq << v.Vector.x << v.Vector.y << v.Vector.z << YAML::EndSeq; break;
				case ScriptFieldType::Vector4: out << YAML::Flow << YAML::BeginSeq << v.Vector.x << v.Vector.y << v.Vector.z << v.Vector.w << YAML::EndSeq; break;
				case ScriptFieldType::Entity:  out << v.Entity; break;
				case ScriptFieldType::String:
				case ScriptFieldType::Enum:    out << v.Text; break;
				}
				out << YAML::EndMap;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap;
	}

	static void ReadScript(const YAML::Node& node, ScriptComponent& script)
	{
		script.ClassName = node["Class"].as<std::string>("");
		const YAML::Node fields = node["Fields"];
		if (!fields || !fields.IsMap())
		{
			return;
		}
		for (const auto& entry : fields)
		{
			const YAML::Node typed = entry.second;
			ScriptFieldValue v;
			if (!typed.IsMap() || typed.size() != 1 || !ScriptFieldValue::ParseTypeName(typed.begin()->first.as<std::string>(), v.Type))
			{
				continue; // not a field this version knows
			}
			const YAML::Node value = typed.begin()->second;
			const size_t components = v.Type == ScriptFieldType::Vector2 ? 2 : v.Type == ScriptFieldType::Vector3 ? 3 : 4;
			switch (v.Type)
			{
			case ScriptFieldType::Bool:   v.Integer = value.as<bool>(false) ? 1 : 0; break;
			case ScriptFieldType::Int:    v.Integer = value.as<int64_t>(0); break;
			case ScriptFieldType::Float:
			case ScriptFieldType::Double: v.Number = value.as<double>(0.0); break;
			case ScriptFieldType::Vector2:
			case ScriptFieldType::Vector3:
			case ScriptFieldType::Vector4:
				for (size_t i = 0; value.IsSequence() && i < components && i < value.size(); i++)
				{
					v.Vector[int(i)] = value[i].as<float>(0.0f);
				}
				break;
			case ScriptFieldType::Entity: v.Entity = value.as<uint64_t>(0); break;
			case ScriptFieldType::String:
			case ScriptFieldType::Enum:   v.Text = value.as<std::string>(""); break;
			}
			script.Fields.push_back({ entry.first.as<std::string>(), v });
		}
	}

	static void WriteEntity(YAML::Emitter& out, EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "ID" << YAML::Value << entity;
		out << YAML::Key << "Name" << YAML::Value << scene.Get<NameComponent>(entity).Name;
		out << YAML::Key << "Parent" << YAML::Value << scene.GetParent(entity);
		if (const TagComponent* tags = scene.TryGet<TagComponent>(entity); tags && !tags->Tags.empty())
		{
			out << YAML::Key << "Tags" << YAML::Value << YAML::Flow << tags->Tags;
		}

		const TransformComponent& transform = scene.Get<TransformComponent>(entity);
		out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
		WriteVec3(out, "Translation", transform.Translation);
		WriteVec3(out, "Rotation", transform.Rotation);
		WriteVec3(out, "Scale", transform.Scale);
		out << YAML::EndMap;

		if (const ModelComponent* model = scene.TryGet<ModelComponent>(entity))
		{
			out << YAML::Key << "Model" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "File" << YAML::Value << ProjectPaths::ToStored(model->FilePath);
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
				out << YAML::Key << "File" << YAML::Value << ProjectPaths::ToStored(part->Material->GetFilePath());
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
		if (const CameraComponent* camera = scene.TryGet<CameraComponent>(entity))
		{
			// Its placement is the entity's transform
			out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Primary" << YAML::Value << camera->Primary;
			out << YAML::Key << "FieldOfView" << YAML::Value << camera->FieldOfView;
			out << YAML::Key << "NearClip" << YAML::Value << camera->NearClip;
			out << YAML::Key << "FarClip" << YAML::Value << camera->FarClip;
			out << YAML::EndMap;
		}
		if (const InputTestComponent* test = scene.TryGet<InputTestComponent>(entity))
		{
			out << YAML::Key << "InputTest" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Speed" << YAML::Value << test->Speed;
			out << YAML::Key << "JumpSpeed" << YAML::Value << test->JumpSpeed;
			out << YAML::Key << "Gravity" << YAML::Value << test->Gravity;
			out << YAML::Key << "TurnSpeed" << YAML::Value << test->TurnSpeed;
			out << YAML::Key << "CaptureMouse" << YAML::Value << test->CaptureMouse;
			out << YAML::EndMap;
		}
		if (const ColliderComponent* collider = scene.TryGet<ColliderComponent>(entity))
		{
			out << YAML::Key << "Collider" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Shape" << YAML::Value << ColliderComponent::GetShapeName(collider->Type);
			WriteVec3(out, "Offset", collider->Offset);
			WriteVec3(out, "HalfExtents", collider->HalfExtents);
			out << YAML::Key << "Radius" << YAML::Value << collider->Radius;
			out << YAML::Key << "HalfHeight" << YAML::Value << collider->HalfHeight;
			out << YAML::Key << "Friction" << YAML::Value << collider->Friction;
			out << YAML::Key << "Restitution" << YAML::Value << collider->Restitution;
			out << YAML::Key << "IsTrigger" << YAML::Value << collider->IsTrigger;
			out << YAML::EndMap;
		}
		if (const RigidBodyComponent* body = scene.TryGet<RigidBodyComponent>(entity))
		{
			out << YAML::Key << "RigidBody" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Type" << YAML::Value << RigidBodyComponent::GetTypeName(body->Type);
			out << YAML::Key << "Mass" << YAML::Value << body->Mass;
			out << YAML::Key << "LinearDamping" << YAML::Value << body->LinearDamping;
			out << YAML::Key << "AngularDamping" << YAML::Value << body->AngularDamping;
			out << YAML::Key << "GravityFactor" << YAML::Value << body->GravityFactor;
			out << YAML::Key << "LockRotation" << YAML::Value << YAML::Flow << YAML::BeginSeq << body->LockRotationX << body->LockRotationY
				<< body->LockRotationZ << YAML::EndSeq;
			out << YAML::Key << "ContinuousCollision" << YAML::Value << body->ContinuousCollision;
			out << YAML::EndMap;
		}
		if (const CharacterControllerComponent* character = scene.TryGet<CharacterControllerComponent>(entity))
		{
			out << YAML::Key << "CharacterController" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Height" << YAML::Value << character->Height;
			out << YAML::Key << "Radius" << YAML::Value << character->Radius;
			WriteVec3(out, "Offset", character->Offset);
			out << YAML::Key << "MaxSlopeDegrees" << YAML::Value << character->MaxSlopeDegrees;
			out << YAML::Key << "StepHeight" << YAML::Value << character->StepHeight;
			out << YAML::Key << "Mass" << YAML::Value << character->Mass;
			out << YAML::Key << "PushStrength" << YAML::Value << character->PushStrength;
			out << YAML::EndMap;
		}
		if (const ScriptComponent* script = scene.TryGet<ScriptComponent>(entity))
		{
			WriteScript(out, *script);
		}
		if (const AudioSourceComponent* source = scene.TryGet<AudioSourceComponent>(entity))
		{
			out << YAML::Key << "AudioSource" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Clip" << YAML::Value << ProjectPaths::ToStored(source->Clip);
			out << YAML::Key << "Bus" << YAML::Value << GetAudioBusName(source->Bus);
			out << YAML::Key << "Volume" << YAML::Value << source->Volume;
			out << YAML::Key << "Pitch" << YAML::Value << source->Pitch;
			out << YAML::Key << "Loop" << YAML::Value << source->Loop;
			out << YAML::Key << "PlayOnStart" << YAML::Value << source->PlayOnStart;
			out << YAML::Key << "Stream" << YAML::Value << source->Stream;
			out << YAML::Key << "Spatial" << YAML::Value << source->Spatial;
			out << YAML::Key << "MinDistance" << YAML::Value << source->MinDistance;
			out << YAML::Key << "MaxDistance" << YAML::Value << source->MaxDistance;
			out << YAML::EndMap;
		}
		if (const AudioListenerComponent* listener = scene.TryGet<AudioListenerComponent>(entity))
		{
			out << YAML::Key << "AudioListener" << YAML::Value << YAML::Flow << YAML::BeginMap;
			out << YAML::Key << "Active" << YAML::Value << listener->Active;
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
			out << YAML::Key << "AutoUpdate" << YAML::Value << p.AutoUpdate;
			out << YAML::Key << "ProbesPerFrame" << YAML::Value << p.ProbesPerFrame;
			out << YAML::Key << "TracedProbesPerFrame" << YAML::Value << p.TracedProbesPerFrame;
			out << YAML::Key << "RaysPerProbe" << YAML::Value << p.RaysPerProbe;
			out << YAML::Key << "Hysteresis" << YAML::Value << p.Hysteresis;
			out << YAML::Key << "RelocateProbes" << YAML::Value << p.RelocateProbes;
			out << YAML::Key << "OccludeReflections" << YAML::Value << p.OccludeReflections;
			out << YAML::Key << "ShowProbes" << YAML::Value << p.ShowProbes;
			out << YAML::Key << "ProbeRadius" << YAML::Value << p.ProbeRadius;
			out << YAML::Key << "Show" << YAML::Value << p.Show;
			out << YAML::EndMap;
		}
		if (const EnvironmentComponent* environment = scene.TryGet<EnvironmentComponent>(entity))
		{
			out << YAML::Key << "Environment" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "File" << YAML::Value << ProjectPaths::ToStored(environment->FilePath);
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

	std::string Serialize(EnvMapVulkanScene& scene, const std::string& sceneName, const EnvMapVulkanRenderSettings& renderSettings,
		const EditorView* editorView)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "Version" << YAML::Value << 1;
		out << YAML::Key << "Name" << YAML::Value << sceneName;
		out << YAML::EndMap;
		if (editorView)
		{
			out << YAML::Key << "EditorCamera" << YAML::Value << YAML::BeginMap;
			WriteVec3(out, "Translation", editorView->Translation);
			WriteVec3(out, "Rotation", editorView->Rotation);
			out << YAML::Key << "FieldOfView" << YAML::Value << editorView->FieldOfView;
			out << YAML::EndMap;
		}
		out << YAML::Key << "RenderSettings" << YAML::Value << YAML::BeginMap;
		renderSettings.Serialize(out);
		out << YAML::EndMap;
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (EnvMapVulkanEntityID root : scene.GetRoots())
		{
			WriteEntity(out, scene, root);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
		return std::string(out.c_str()) + "\n";
	}

	bool ReadEditorView(const std::string& filepath, EditorView& view)
	{
		try
		{
			const YAML::Node camera = YAML::LoadFile(filepath)["EditorCamera"];
			if (!camera || !camera.IsMap())
			{
				return false;
			}
			ReadVec3(camera["Translation"], view.Translation);
			ReadVec3(camera["Rotation"], view.Rotation);
			Read(camera, "FieldOfView", view.FieldOfView);
			return true;
		}
		catch (const std::exception&)
		{
			return false; // Deserialize says what is wrong with the file
		}
	}

	bool Deserialize(const std::string& filepath, EnvMapVulkanScene& scene, std::string& sceneName, EnvMapVulkanRenderSettings& renderSettings,
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
		renderSettings = EnvMapVulkanRenderSettings::Deserialize(root["RenderSettings"]);

		try
		{
			for (const YAML::Node& node : root["Entities"])
			{
				const EnvMapVulkanEntityID id = node["ID"].as<uint64_t>(NoEntity);
				const EnvMapVulkanEntityID parent = node["Parent"].as<uint64_t>(NoEntity);
				const EnvMapVulkanEntityID entity = scene.CreateEntity(node["Name"].as<std::string>("Entity"), parent, id);
				if (YAML::Node tagsNode = node["Tags"]; tagsNode && tagsNode.IsSequence())
				{
					TagComponent& tags = scene.Add<TagComponent>(entity);
					for (const YAML::Node& tag : tagsNode)
					{
						tags.Add(tag.as<std::string>(""));
					}
				}

				if (YAML::Node transformNode = node["Transform"])
				{
					TransformComponent& transform = scene.Get<TransformComponent>(entity);
					ReadVec3(transformNode["Translation"], transform.Translation);
					ReadVec3(transformNode["Rotation"], transform.Rotation);
					ReadVec3(transformNode["Scale"], transform.Scale);
				}
				if (YAML::Node modelNode = node["Model"])
				{
					scene.Add<ModelComponent>(entity).FilePath = ProjectPaths::Resolve(modelNode["File"].as<std::string>(""));
				}
				if (YAML::Node partNode = node["MeshPart"])
				{
					MeshPartComponent& part = scene.Add<MeshPartComponent>(entity);
					part.SourceMeshIndex = partNode["SourceMeshIndex"].as<uint32_t>(0);
					part.MeshIndex = part.SourceMeshIndex; // the caller sets it once the model's meshes are known
					if (YAML::Node materialNode = partNode["Material"])
					{
						materialRefs[entity] = { materialNode["ID"].as<uint64_t>(0), ProjectPaths::Resolve(materialNode["File"].as<std::string>("")) };
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
				if (YAML::Node cameraNode = node["Camera"])
				{
					// Scenes saved before 6.3: the one camera, primary, with the planes every view had
					CameraComponent& camera = scene.Add<CameraComponent>(entity);
					Read(cameraNode, "Primary", camera.Primary);
					Read(cameraNode, "FieldOfView", camera.FieldOfView); // scenes saved before the camera had a lens: 45
					Read(cameraNode, "NearClip", camera.NearClip);
					Read(cameraNode, "FarClip", camera.FarClip);
				}
				if (YAML::Node testNode = node["InputTest"])
				{
					InputTestComponent& test = scene.Add<InputTestComponent>(entity);
					Read(testNode, "Speed", test.Speed);
					Read(testNode, "JumpSpeed", test.JumpSpeed);
					Read(testNode, "Gravity", test.Gravity);
					Read(testNode, "TurnSpeed", test.TurnSpeed);
					Read(testNode, "CaptureMouse", test.CaptureMouse);
				}
				if (YAML::Node colliderNode = node["Collider"])
				{
					ColliderComponent& collider = scene.Add<ColliderComponent>(entity);
					const std::string shape = colliderNode["Shape"].as<std::string>("Box");
					for (ColliderComponent::Shape s : { ColliderComponent::Shape::Box, ColliderComponent::Shape::Sphere, ColliderComponent::Shape::Capsule,
						ColliderComponent::Shape::ConvexMesh, ColliderComponent::Shape::TriangleMesh })
					{
						if (shape == ColliderComponent::GetShapeName(s))
						{
							collider.Type = s;
						}
					}
					ReadVec3(colliderNode["Offset"], collider.Offset);
					ReadVec3(colliderNode["HalfExtents"], collider.HalfExtents);
					Read(colliderNode, "Radius", collider.Radius);
					Read(colliderNode, "HalfHeight", collider.HalfHeight);
					Read(colliderNode, "Friction", collider.Friction);
					Read(colliderNode, "Restitution", collider.Restitution);
					Read(colliderNode, "IsTrigger", collider.IsTrigger);
				}
				if (YAML::Node bodyNode = node["RigidBody"])
				{
					RigidBodyComponent& body = scene.Add<RigidBodyComponent>(entity);
					const std::string type = bodyNode["Type"].as<std::string>("Dynamic");
					for (RigidBodyComponent::BodyType t : { RigidBodyComponent::BodyType::Static, RigidBodyComponent::BodyType::Dynamic,
						RigidBodyComponent::BodyType::Kinematic })
					{
						if (type == RigidBodyComponent::GetTypeName(t))
						{
							body.Type = t;
						}
					}
					Read(bodyNode, "Mass", body.Mass);
					Read(bodyNode, "LinearDamping", body.LinearDamping);
					Read(bodyNode, "AngularDamping", body.AngularDamping);
					Read(bodyNode, "GravityFactor", body.GravityFactor);
					if (YAML::Node locks = bodyNode["LockRotation"]; locks && locks.IsSequence() && locks.size() == 3)
					{
						body.LockRotationX = locks[0].as<bool>();
						body.LockRotationY = locks[1].as<bool>();
						body.LockRotationZ = locks[2].as<bool>();
					}
					Read(bodyNode, "ContinuousCollision", body.ContinuousCollision);
				}
				if (YAML::Node characterNode = node["CharacterController"])
				{
					CharacterControllerComponent& character = scene.Add<CharacterControllerComponent>(entity);
					Read(characterNode, "Height", character.Height);
					Read(characterNode, "Radius", character.Radius);
					ReadVec3(characterNode["Offset"], character.Offset);
					Read(characterNode, "MaxSlopeDegrees", character.MaxSlopeDegrees);
					Read(characterNode, "StepHeight", character.StepHeight);
					Read(characterNode, "Mass", character.Mass);
					Read(characterNode, "PushStrength", character.PushStrength);
				}
				if (YAML::Node scriptNode = node["Script"]; scriptNode && scriptNode.IsMap())
				{
					ReadScript(scriptNode, scene.Add<ScriptComponent>(entity));
				}
				if (YAML::Node sourceNode = node["AudioSource"]; sourceNode && sourceNode.IsMap())
				{
					AudioSourceComponent& source = scene.Add<AudioSourceComponent>(entity);
					source.Clip = ProjectPaths::Resolve(sourceNode["Clip"].as<std::string>(""));
					if (!FindAudioBus(sourceNode["Bus"].as<std::string>("Effects").c_str(), source.Bus))
					{
						source.Bus = AudioBus::Effects;
					}
					Read(sourceNode, "Volume", source.Volume);
					Read(sourceNode, "Pitch", source.Pitch);
					Read(sourceNode, "Loop", source.Loop);
					Read(sourceNode, "PlayOnStart", source.PlayOnStart);
					Read(sourceNode, "Stream", source.Stream);
					Read(sourceNode, "Spatial", source.Spatial);
					Read(sourceNode, "MinDistance", source.MinDistance);
					Read(sourceNode, "MaxDistance", source.MaxDistance);
				}
				if (YAML::Node listenerNode = node["AudioListener"]; listenerNode && listenerNode.IsMap())
				{
					Read(listenerNode, "Active", scene.Add<AudioListenerComponent>(entity).Active);
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
					Read(probesNode, "AutoUpdate", p.AutoUpdate);
					Read(probesNode, "ProbesPerFrame", p.ProbesPerFrame);
					Read(probesNode, "TracedProbesPerFrame", p.TracedProbesPerFrame);
					Read(probesNode, "RaysPerProbe", p.RaysPerProbe);
					Read(probesNode, "Hysteresis", p.Hysteresis);
					Read(probesNode, "RelocateProbes", p.RelocateProbes);
					Read(probesNode, "OccludeReflections", p.OccludeReflections);
					Read(probesNode, "ShowProbes", p.ShowProbes);
					Read(probesNode, "ProbeRadius", p.ProbeRadius);
					Read(probesNode, "Show", p.Show);
				}
				if (YAML::Node environmentNode = node["Environment"])
				{
					EnvironmentComponent& environment = scene.Add<EnvironmentComponent>(entity);
					Read(environmentNode, "File", environment.FilePath);
					environment.FilePath = ProjectPaths::Resolve(environment.FilePath);
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
