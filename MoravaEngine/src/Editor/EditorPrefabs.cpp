// Prefabs in the editor (docs/engine/Game_Engine_Runtime_Guide.html, 6.1): saving an entity as a prefab, placing one,
// Apply, Revert and Unpack, and bringing the instances of a changed prefab up to date when a scene opens. The requests
// are queued and done at the start of the next frame (ProcessPrefabOperations), with the model loading and removing:
// replacing an instance destroys models that frames in flight may still draw.


#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkan/EnvMapVulkanPrefab.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanSceneSerializer.h"

#include "Project/Project.h"

#include "Core/FileDialogs.h"
#include "Core/Log.h"

#include "imgui.h"

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <vector>


#include "EditorState.h"


namespace fs = std::filesystem;

struct PendingPrefabOperation
{
	PrefabOperation Kind;
	EnvMapVulkanEntityID Entity = NoEntity;
	std::string File;
	std::optional<glm::vec3> Position; // Place: where (none: where the camera looks)
};
static std::vector<PendingPrefabOperation> s_PendingPrefabOperations;

void RequestPrefabOperation(PrefabOperation kind, EnvMapVulkanEntityID entity, const std::string& file, std::optional<glm::vec3> position)
{
	s_PendingPrefabOperations.push_back({ kind, entity, file, position });
}

bool IsPrefabFile(const std::string& path)
{
	return fs::u8path(path).extension() == EnvMapVulkanSceneSerializer::PrefabExtension;
}

EnvMapVulkanEntityID FindPrefabInstanceRoot(EnvMapVulkanEntityID entity)
{
	for (EnvMapVulkanEntityID at = entity; at != NoEntity; at = s_Scene.GetParent(at))
	{
		if (s_Scene.Has<PrefabComponent>(at))
		{
			return at;
		}
	}
	return NoEntity;
}

std::string GetPrefabsFolder()
{
	const Project* project = Project::GetOpen();
	return project ? (fs::u8path(project->GetAssetFolder()) / EnvMapVulkanSceneSerializer::PrefabsFolder).generic_u8string() : std::string();
}

std::string GetNewPrefabFile(const std::string& folder, const std::string& entityName)
{
	// The entity's name as a file name (no spaces: MakeFileName), and a number when that file exists
	const std::string base = EnvMapVulkanMaterialLibrary::MakeFileName(entityName, "Prefab");
	std::error_code error;
	fs::path path = fs::u8path(folder) / (base + EnvMapVulkanSceneSerializer::PrefabExtension);
	for (int i = 2; fs::exists(path, error); i++)
	{
		path = fs::u8path(folder) / (base + "_" + std::to_string(i) + EnvMapVulkanSceneSerializer::PrefabExtension);
	}
	return path.generic_u8string();
}

void SaveAsPrefabWithDialog(EnvMapVulkanEntityID entity)
{
	if (!s_Scene.Exists(entity))
	{
		return;
	}
	const std::string folder = GetPrefabsFolder();
	std::error_code error;
	if (!folder.empty())
	{
		fs::create_directories(fs::u8path(folder), error);
	}
	const std::string file = FileDialogs::SaveFile({ { "Prefab", "mprefab" } }, folder.empty() ? GetAssetFolderForDialog() : folder);
	if (!file.empty())
	{
		RequestPrefabOperation(PrefabOperation::Save, entity, file);
	}
}

// The instances of the prefab in the open scene that didn't change since they matched it take its new content; the ones
// that did keep theirs. Returns how many were updated, and how many kept (keptChanged)
static int UpdateUntouchedInstances(const std::string& file, EnvMapVulkanEntityID except, int& keptChanged)
{
	int updated = 0;
	keptChanged = 0;
	for (EnvMapVulkanEntityID instance : EnvMapVulkanPrefab::FindInstances(s_Scene, file))
	{
		if (instance == except || !s_Scene.Exists(instance))
		{
			continue;
		}
		const EnvMapVulkanPrefab::InstanceState state = EnvMapVulkanPrefab::GetState(s_Scene, instance);
		if (state == EnvMapVulkanPrefab::InstanceState::PrefabChanged)
		{
			std::string error;
			if (EnvMapVulkanPrefab::Revert(s_Scene, instance, error) != NoEntity)
			{
				updated++;
			}
			else
			{
				Log::GetLogger()->error("Prefab: '{0}' could not be updated: {1}", s_Scene.Get<NameComponent>(instance).Name, error);
			}
		}
		else if (state == EnvMapVulkanPrefab::InstanceState::BothChanged)
		{
			keptChanged++;
		}
	}
	return updated;
}

void ProcessPrefabOperations()
{
	if (s_PendingPrefabOperations.empty())
	{
		return;
	}
	const std::vector<PendingPrefabOperation> operations = std::move(s_PendingPrefabOperations);
	s_PendingPrefabOperations.clear();
	if (RefusedWhilePlaying("Changing prefabs"))
	{
		return;
	}
	// Replacing instances destroys models that frames in flight may draw
	EnvMapVulkanRenderer::WaitIdle();
	for (const PendingPrefabOperation& operation : operations)
	{
		std::string error;
		switch (operation.Kind)
		{
			case PrefabOperation::Save:
			{
				if (!s_Scene.Exists(operation.Entity))
				{
					break;
				}
				if (EnvMapVulkanPrefab::Save(s_Scene, operation.Entity, operation.File, error))
				{
					Log::GetLogger()->info("Prefab '{0}' saved from '{1}'", operation.File, s_Scene.Get<NameComponent>(operation.Entity).Name);
					ShowNotification("Prefab saved: " + fs::u8path(operation.File).filename().u8string());
				}
				else
				{
					ShowNotification("The prefab could not be saved: " + error, true);
				}
				break;
			}
			case PrefabOperation::Place:
			{
				const EnvMapVulkanEntityID instance = EnvMapVulkanPrefab::Instantiate(s_Scene, operation.File, NoEntity, error);
				if (instance == NoEntity)
				{
					Log::GetLogger()->error("Prefab '{0}' could not be placed: {1}", operation.File, error);
					ShowNotification("The prefab could not be placed: " + error, true);
					break;
				}
				s_Scene.Get<TransformComponent>(instance).Translation = operation.Position ? *operation.Position : GetLightSpawnTarget();
				s_EditorSelection.SelectOnly(instance);
				Log::GetLogger()->info("Prefab '{0}' placed: '{1}'", operation.File, s_Scene.Get<NameComponent>(instance).Name);
				break;
			}
			case PrefabOperation::Apply:
			{
				const PrefabComponent* prefab = s_Scene.TryGet<PrefabComponent>(operation.Entity);
				if (!prefab)
				{
					break;
				}
				const std::string file = prefab->File;
				if (!EnvMapVulkanPrefab::Save(s_Scene, operation.Entity, file, error))
				{
					ShowNotification("The prefab could not be saved: " + error, true);
					break;
				}
				int kept = 0;
				const int updated = UpdateUntouchedInstances(file, operation.Entity, kept);
				std::string text = "Applied to " + fs::u8path(file).filename().u8string();
				if (updated > 0)
				{
					text += ": " + std::to_string(updated) + " other instance(s) updated";
				}
				if (kept > 0)
				{
					text += (updated > 0 ? ", " : ": ") + std::to_string(kept) + " with changes of their own kept (Revert takes the prefab)";
				}
				Log::GetLogger()->info("Prefab: {0}", text);
				ShowNotification(text);
				break;
			}
			case PrefabOperation::Revert:
			{
				if (EnvMapVulkanPrefab::Revert(s_Scene, operation.Entity, error) == NoEntity)
				{
					ShowNotification("The instance could not be reverted: " + error, true);
				}
				break;
			}
			case PrefabOperation::Unpack:
			{
				if (s_Scene.Has<PrefabComponent>(operation.Entity))
				{
					s_Scene.Remove<PrefabComponent>(operation.Entity);
				}
				break;
			}
		}
	}
	s_EditorSelection.Sync();
	ExtractLights(); // a prefab may have lights
}

void UpdatePrefabInstancesOfOpenedScene()
{
	// Each prefab once: its instances that didn't change since they matched it take its new content. A link written by
	// hand (no hashes) is taken as matching the prefab as it is
	std::vector<std::string> files;
	s_Scene.Each<PrefabComponent>([&](EnvMapVulkanEntityID entity, PrefabComponent& prefab)
	{
		if (prefab.PrefabHash == 0 && prefab.InstanceHash == 0)
		{
			EnvMapVulkanPrefab::GetFileHash(prefab.File, prefab.PrefabHash);
			prefab.InstanceHash = EnvMapVulkanPrefab::GetInstanceHash(s_Scene, entity);
		}
		const std::string file = ProjectPaths::Normalize(prefab.File);
		if (std::find(files.begin(), files.end(), file) == files.end())
		{
			files.push_back(file);
		}
	});
	if (files.empty())
	{
		return;
	}
	EnvMapVulkanRenderer::WaitIdle();
	int updated = 0, kept = 0;
	for (const std::string& file : files)
	{
		int keptHere = 0;
		updated += UpdateUntouchedInstances(file, NoEntity, keptHere);
		kept += keptHere;
	}
	if (updated > 0 || kept > 0)
	{
		Log::GetLogger()->info("Prefabs: {0} instance(s) updated to their changed prefab, {1} with changes of their own kept", updated, kept);
	}
	if (updated > 0)
	{
		ShowNotification(std::to_string(updated) + " prefab instance(s) updated to their changed prefab (save the scene to keep it)");
	}
}

void PrefabControls(EnvMapVulkanEntityID entity)
{
	const EnvMapVulkanEntityID root = FindPrefabInstanceRoot(entity);
	if (root == NoEntity)
	{
		return;
	}
	const PrefabComponent& prefab = s_Scene.Get<PrefabComponent>(root);
	const std::string fileName = fs::u8path(prefab.File).filename().u8string();
	ImGui::PushID("Prefab");
	if (root != entity)
	{
		// A part of an instance: the instance is where the prefab's tools are
		ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "Part of the prefab instance '%s'", s_Scene.Get<NameComponent>(root).Name.c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Select It"))
		{
			s_EditorSelection.SelectOnly(root);
		}
		ImGui::PopID();
		ImGui::Separator();
		return;
	}

	const EnvMapVulkanPrefab::InstanceState state = EnvMapVulkanPrefab::GetState(s_Scene, root);
	ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "Prefab: %s", fileName.c_str());
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s\nThe instance's translation and rotation are its own (where it is placed); the rest is the prefab's",
			prefab.File.c_str());
	}
	ImGui::SameLine();
	const bool inSync = state == EnvMapVulkanPrefab::InstanceState::InSync;
	ImGui::TextDisabled("(%s)", EnvMapVulkanPrefab::GetStateName(state));

	ImGui::BeginDisabled(IsPlaying());
	ImGui::BeginDisabled(state == EnvMapVulkanPrefab::InstanceState::FileMissing ? false : inSync);
	if (ImGui::Button("Apply to Prefab"))
	{
		RequestPrefabOperation(PrefabOperation::Apply, root);
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Writes this instance to %s (its placement aside). The other instances in this scene that\n"
			"didn't change take it; the ones that did keep their changes (their Revert takes the prefab).%s", fileName.c_str(),
			state == EnvMapVulkanPrefab::InstanceState::BothChanged ? "\nThe prefab changed too: its changes are replaced by this instance's." : "");
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(inSync || state == EnvMapVulkanPrefab::InstanceState::FileMissing);
	if (ImGui::Button("Revert"))
	{
		RequestPrefabOperation(PrefabOperation::Revert, root);
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("The instance as the prefab is, where it is: its changes are lost. Its entities keep their IDs,\n"
			"so scripts that point at them still do.");
	}
	ImGui::SameLine();
	if (ImGui::Button("Unpack"))
	{
		RequestPrefabOperation(PrefabOperation::Unpack, root);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Ordinary entities from now on: no longer an instance of %s", fileName.c_str());
	}
	ImGui::EndDisabled();
	ImGui::PopID();
	ImGui::Separator();
}
