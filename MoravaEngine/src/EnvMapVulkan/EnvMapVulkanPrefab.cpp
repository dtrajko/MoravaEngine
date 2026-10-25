#include "EnvMapVulkanPrefab.h"

#include "EnvMapVulkanSceneLoader.h"
#include "EnvMapVulkanSceneSerializer.h"

#include "H2M/Renderer/ModelH2M.h" // a scene's copy shares the models: destroying it releases them

#include "Core/Log.h"

#include "Project/Project.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unordered_map>


namespace EnvMapVulkanPrefab
{
	const char* GetStateName(InstanceState state)
	{
		switch (state)
		{
			case InstanceState::InSync:          return "as the prefab";
			case InstanceState::InstanceChanged: return "changed here";
			case InstanceState::PrefabChanged:   return "the prefab changed";
			case InstanceState::BothChanged:     return "changed here, and the prefab changed";
			case InstanceState::FileMissing:     return "the prefab file is missing";
			default:                             return "?";
		}
	}

	std::string GetPrefabText(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID root, const std::string& name)
	{
		EnvMapVulkanScene prefab;
		const EnvMapVulkanEntityID copy = prefab.CopySubtree(scene, root, NoEntity, [](size_t i) { return EnvMapVulkanEntityID(i + 1); });
		if (copy == NoEntity)
		{
			return std::string();
		}
		// The root's name, translation and rotation are an instance's own: the prefab's root has the prefab's name
		prefab.Get<NameComponent>(copy).Name = name;
		TransformComponent& transform = prefab.Get<TransformComponent>(copy);
		transform.Translation = glm::vec3(0.0f);
		transform.Rotation = glm::vec3(0.0f);
		if (prefab.Has<PrefabComponent>(copy))
		{
			prefab.Remove<PrefabComponent>(copy);
		}
		return EnvMapVulkanSceneSerializer::SerializePrefab(prefab, name);
	}

	uint64_t HashText(const std::string& text)
	{
		uint64_t hash = 14695981039346656037ull;
		for (char c : text)
		{
			if (c == '\r')
			{
				continue;
			}
			hash ^= (unsigned char)c;
			hash *= 1099511628211ull;
		}
		return hash;
	}

	uint64_t GetInstanceHash(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID root)
	{
		return HashText(GetPrefabText(scene, root, std::string()));
	}

	bool GetFileHash(const std::string& file, uint64_t& hash)
	{
		// Read again only when the file changed (the Inspector asks for the selected instance's state every frame)
		struct Cached
		{
			std::filesystem::file_time_type WriteTime;
			uint64_t Hash = 0;
		};
		static std::unordered_map<std::string, Cached> s_Cache;
		const std::filesystem::path path = std::filesystem::u8path(file);
		std::error_code error;
		const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(path, error);
		if (error)
		{
			return false;
		}
		if (auto it = s_Cache.find(file); it != s_Cache.end() && it->second.WriteTime == writeTime)
		{
			hash = it->second.Hash;
			return true;
		}
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
		{
			return false;
		}
		std::stringstream text;
		text << stream.rdbuf();
		hash = HashText(text.str());
		s_Cache[file] = { writeTime, hash };
		return true;
	}

	bool Save(EnvMapVulkanScene& scene, EnvMapVulkanEntityID root, const std::string& file, std::string& error)
	{
		const std::filesystem::path path = std::filesystem::u8path(file);
		const std::string text = GetPrefabText(scene, root, path.stem().u8string());
		std::error_code fileError;
		if (!path.parent_path().empty())
		{
			std::filesystem::create_directories(path.parent_path(), fileError);
		}
		std::ofstream stream(path, std::ios::binary);
		if (!stream || !(stream << text))
		{
			error = "'" + file + "' can't be written";
			return false;
		}
		stream.close();
		PrefabComponent& prefab = scene.Has<PrefabComponent>(root) ? scene.Get<PrefabComponent>(root) : scene.Add<PrefabComponent>(root);
		prefab.File = path.lexically_normal().generic_u8string();
		prefab.PrefabHash = HashText(text);
		prefab.InstanceHash = GetInstanceHash(scene, root);
		return true;
	}

	EnvMapVulkanEntityID Instantiate(EnvMapVulkanScene& scene, const std::string& file, EnvMapVulkanEntityID parent, std::string& error,
		const std::vector<EnvMapVulkanEntityID>& reuseIds)
	{
		std::unordered_map<EnvMapVulkanEntityID, EnvMapVulkanSceneSerializer::MaterialRef> materialRefs;
		std::vector<EnvMapVulkanEntityID> created;
		auto newId = [&](size_t i) { return i < reuseIds.size() ? reuseIds[i] : NoEntity; };
		if (!EnvMapVulkanSceneSerializer::DeserializePrefab(file, scene, parent, newId, materialRefs, created, error))
		{
			return NoEntity;
		}
		// The models, as an opened scene's (the parts are there: they get their meshes and materials)
		for (EnvMapVulkanEntityID entity : created)
		{
			if (scene.Exists(entity) && scene.Has<ModelComponent>(entity) && !EnvMapVulkanSceneLoader::AttachModel(scene, entity, materialRefs))
			{
				Log::GetLogger()->error("Prefab '{0}': the model of '{1}' could not be loaded: left out", file, scene.Get<NameComponent>(entity).Name);
				scene.DestroyEntity(entity);
			}
		}
		const EnvMapVulkanEntityID root = created.front();
		if (!scene.Exists(root))
		{
			error = "its root's model could not be loaded";
			return NoEntity;
		}
		uint64_t fileHash = 0;
		GetFileHash(file, fileHash);
		PrefabComponent& prefab = scene.Has<PrefabComponent>(root) ? scene.Get<PrefabComponent>(root) : scene.Add<PrefabComponent>(root);
		prefab.File = std::filesystem::u8path(file).lexically_normal().generic_u8string();
		prefab.PrefabHash = fileHash;
		prefab.InstanceHash = GetInstanceHash(scene, root);
		return root;
	}

	InstanceState GetState(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID root)
	{
		const PrefabComponent* prefab = scene.TryGet<PrefabComponent>(root);
		uint64_t fileHash = 0;
		if (!prefab || !GetFileHash(prefab->File, fileHash))
		{
			return InstanceState::FileMissing;
		}
		const bool prefabChanged = fileHash != prefab->PrefabHash;
		const bool instanceChanged = GetInstanceHash(scene, root) != prefab->InstanceHash;
		return prefabChanged ? (instanceChanged ? InstanceState::BothChanged : InstanceState::PrefabChanged) :
			(instanceChanged ? InstanceState::InstanceChanged : InstanceState::InSync);
	}

	EnvMapVulkanEntityID Revert(EnvMapVulkanScene& scene, EnvMapVulkanEntityID root, std::string& error)
	{
		const PrefabComponent* prefab = scene.TryGet<PrefabComponent>(root);
		if (!prefab)
		{
			error = "it isn't a prefab's instance";
			return NoEntity;
		}
		const std::string file = prefab->File;
		const EnvMapVulkanEntityID parent = scene.GetParent(root);
		const TransformComponent placement = scene.Get<TransformComponent>(root);
		const std::string rootName = scene.Get<NameComponent>(root).Name;
		const std::vector<EnvMapVulkanEntityID>& siblings = parent == NoEntity ? scene.GetRoots() : scene.GetChildren(parent);
		const int index = (int)(std::find(siblings.begin(), siblings.end(), root) - siblings.begin());
		const std::vector<EnvMapVulkanEntityID> ids = scene.GetSubtree(root);

		// The new one is read before the old one goes: a prefab that can't be read leaves the instance as it was. Its IDs
		// are the old ones, so the old entities make way first, and come back if reading fails
		EnvMapVulkanScene kept;
		kept.CopySubtree(scene, root, NoEntity, [&](size_t i) { return ids[i]; });
		scene.DestroyEntity(root);
		const EnvMapVulkanEntityID instance = Instantiate(scene, file, parent, error, ids);
		if (instance == NoEntity)
		{
			scene.CopySubtree(kept, root, parent, [&](size_t i) { return ids[i]; });
			scene.SetParent(root, parent, false, index);
			return NoEntity;
		}
		scene.Get<NameComponent>(instance).Name = rootName;
		TransformComponent& transform = scene.Get<TransformComponent>(instance);
		transform.Translation = placement.Translation;
		transform.Rotation = placement.Rotation;
		scene.Get<PrefabComponent>(instance).InstanceHash = GetInstanceHash(scene, instance);
		scene.SetParent(instance, parent, false, index);
		return instance;
	}

	std::vector<EnvMapVulkanEntityID> FindInstances(const EnvMapVulkanScene& scene, const std::string& file)
	{
		const std::string wanted = ProjectPaths::Normalize(file);
		std::vector<EnvMapVulkanEntityID> instances;
		scene.Each<PrefabComponent>([&](EnvMapVulkanEntityID entity, const PrefabComponent& prefab)
		{
			if (ProjectPaths::Normalize(prefab.File) == wanted)
			{
				instances.push_back(entity);
			}
		});
		return instances;
	}

	bool SelfTest()
	{
		const std::filesystem::path path = std::filesystem::temp_directory_path() / "MoravaPrefabSelfTest.mprefab";
		const std::string file = path.generic_u8string();
		auto fail = [&](const char* what)
		{
			std::error_code error;
			std::filesystem::remove(path, error);
			Log::GetLogger()->error("EnvMapVulkanPrefab self-test failed: {0}", what);
			return false;
		};

		EnvMapVulkanScene scene;
		const EnvMapVulkanEntityID thing = scene.CreateEntity("Thing");
		const EnvMapVulkanEntityID part = scene.CreateEntity("Part", thing);
		TransformComponent& placement = scene.Get<TransformComponent>(thing);
		placement.Translation = glm::vec3(1.0f, 2.0f, 3.0f);
		placement.Rotation = glm::vec3(0.0f, 30.0f, 0.0f);
		placement.Scale = glm::vec3(2.0f);
		scene.Add<TagComponent>(part).Add("Red");
		ScriptFieldValue target;
		target.Type = ScriptFieldType::Entity;
		target.Entity = part;
		scene.Add<ScriptComponent>(thing).SetField("Target", target);

		std::string error;
		if (!Save(scene, thing, file, error) || !scene.Has<PrefabComponent>(thing) || GetState(scene, thing) != InstanceState::InSync)
		{
			return fail("save");
		}
		const EnvMapVulkanEntityID instance = Instantiate(scene, file, NoEntity, error);
		if (instance == NoEntity || scene.GetSubtree(instance).size() != 2 || scene.Get<NameComponent>(instance).Name != "MoravaPrefabSelfTest")
		{
			return fail("instantiate");
		}
		const EnvMapVulkanEntityID instancePart = scene.GetChildren(instance).front();
		if (scene.Get<TransformComponent>(instance).Translation != glm::vec3(0.0f) || scene.Get<TransformComponent>(instance).Scale != glm::vec3(2.0f) ||
			scene.Get<ScriptComponent>(instance).FindField("Target")->Entity != instancePart || !scene.Get<TagComponent>(instancePart).Has("Red"))
		{
			return fail("instantiate: placement, scale, reference into the copy, components");
		}

		// Its name, translation and rotation are its own: no change
		scene.Get<NameComponent>(instance).Name = "Thing 2";
		scene.Get<TransformComponent>(instance).Translation = glm::vec3(5.0f, 0.0f, 0.0f);
		scene.Get<TransformComponent>(instance).Rotation = glm::vec3(0.0f, 90.0f, 0.0f);
		if (GetState(scene, instance) != InstanceState::InSync)
		{
			return fail("placement isn't a change");
		}
		scene.Get<TagComponent>(instancePart).Add("Blue");
		if (GetState(scene, instance) != InstanceState::InstanceChanged)
		{
			return fail("a change of the instance");
		}
		if (Revert(scene, instance, error) != instance || scene.GetChildren(instance).front() != instancePart ||
			scene.Get<TagComponent>(instancePart).Has("Blue") || scene.Get<NameComponent>(instance).Name != "Thing 2" ||
			scene.Get<TransformComponent>(instance).Translation != glm::vec3(5.0f, 0.0f, 0.0f) || GetState(scene, instance) != InstanceState::InSync)
		{
			return fail("revert: IDs, content, name and placement");
		}

		// Apply from the first: the second, unchanged, sees the prefab changed and takes it
		scene.Get<TagComponent>(part).Add("Green");
		if (!Save(scene, thing, file, error) || GetState(scene, instance) != InstanceState::PrefabChanged)
		{
			return fail("apply: the other instance sees the prefab changed");
		}
		if (Revert(scene, instance, error) != instance || !scene.Get<TagComponent>(scene.GetChildren(instance).front()).Has("Green") ||
			GetState(scene, instance) != InstanceState::InSync)
		{
			return fail("apply: the other instance takes it");
		}
		// Both changed
		scene.Get<TagComponent>(scene.GetChildren(instance).front()).Add("Mine");
		scene.Get<TagComponent>(part).Remove("Red");
		if (!Save(scene, thing, file, error) || GetState(scene, instance) != InstanceState::BothChanged)
		{
			return fail("both changed");
		}
		if (FindInstances(scene, file).size() != 2)
		{
			return fail("find instances");
		}

		std::error_code removeError;
		std::filesystem::remove(path, removeError);
		if (GetState(scene, thing) != InstanceState::FileMissing)
		{
			return fail("file missing");
		}
		Log::GetLogger()->info("EnvMapVulkanPrefab self-test passed");
		return true;
	}
}
