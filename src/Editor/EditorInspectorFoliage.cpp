// The Inspector's Foliage section, below a terrain's (FoliageComponent, World/Foliage.h; the racing game's Phase 8): the species
// (each a model and the rules for where its copies stand) and the scatter's settings. The renderer scatters again when a
// setting that moves the copies changes (EnvMapVulkanFoliage::Update): those fields change the foliage when their edit ends
// (a drag let go, a number typed), as the terrain's shape does; the distances change it at once. Scatter Again scatters with
// the same settings: after a collider of the keep-out moved, which the settings don't show.


#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"
#include "Core/FileDialogs.h"
#include "Core/Util.h"
#include "Project/Project.h"

#include "imgui.h"

#include <cstdio>
#include <filesystem>
#include <string>


#include "EditorState.h"


// The settings being edited, for the selected terrain's foliage (written to it when an edit ends)
static FoliageSettings s_EditedFoliage;
static EnvMapVulkanEntityID s_EditedFoliageEntity = NoEntity;

static void FoliageTooltip(const char* text)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("%s", text);
	}
}

// A species' model (or its impostor's): shown by its path in the project, dropped from the Content Browser or picked in a dialog
static bool SpeciesModelControl(std::string& model, const char* id, const char* button, const char* empty, const char* tooltip)
{
	bool changed = false;
	const std::string shown = model.empty() ? std::string(empty) : ProjectPaths::ToStored(model);
	char buffer[512] = {};
	snprintf(buffer, sizeof(buffer), "%s", shown.c_str());
	ImGui::SetNextItemWidth(-90.0f);
	ImGui::BeginDisabled(true);
	ImGui::InputText(id, buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			const std::string path = std::filesystem::u8path(Util::to_str((const wchar_t*)payload->Data)).lexically_normal().generic_u8string();
			if (EnvMapVulkanSceneLoader::IsModelFile(path))
			{
				model = ProjectPaths::Normalize(path);
				changed = true;
			}
			else
			{
				ShowNotification("A species is a model file (glTF, FBX, OBJ...)", true);
			}
		}
		ImGui::EndDragDropTarget();
	}
	FoliageTooltip(tooltip);
	ImGui::SameLine();
	if (ImGui::Button(button))
	{
		const std::string file = FileDialogs::OpenFile({}, GetAssetFolderForDialog());
		if (!file.empty())
		{
			if (EnvMapVulkanSceneLoader::IsModelFile(file))
			{
				model = ProjectPaths::Normalize(file);
				changed = true;
			}
			else
			{
				ShowNotification("A species is a model file (glTF, FBX, OBJ...)", true);
			}
		}
	}
	return changed;
}

void FoliageControls(EnvMapVulkanEntityID entity)
{
	FoliageComponent* foliage = s_Scene.TryGet<FoliageComponent>(entity);
	ImGui::SeparatorText("Foliage");
	if (!foliage)
	{
		if (ImGui::Button("Add Foliage"))
		{
			FoliageComponent& added = s_Scene.Add<FoliageComponent>(entity);
			added.Settings.Seed = Terrain::RandomSeed();
		}
		FoliageTooltip("Trees, bushes or rocks scattered over this terrain: species of models with rules for where they stand\n"
			"(density, slope, height, how far from the road and other colliders). Drawn only: no colliders");
		return;
	}
	if (s_EditedFoliageEntity != entity || !ImGui::IsAnyItemActive())
	{
		s_EditedFoliage = foliage->Settings; // nothing being edited: what the component has
		s_EditedFoliageEntity = entity;
	}
	bool commit = false; // an edit of the scatter ended: the copies are placed again
	auto scatterEdited = [&] { commit = commit || ImGui::IsItemDeactivatedAfterEdit(); };
	FoliageSettings& s = s_EditedFoliage;

	ImGui::Checkbox("Enabled##Foliage", &foliage->Enabled);
	FoliageTooltip("Off: no copies drawn (the settings stay)");
	ImGui::SameLine();
	if (ImGui::Button("Scatter Again"))
	{
		foliage->Instances.reset(); // the renderer scatters at the next frame
	}
	FoliageTooltip("Places the copies again with the same settings: after a road or another collider of the keep-out moved\n"
		"(the settings don't change with it)");
	ImGui::SameLine();
	if (ImGui::Button("Remove Foliage"))
	{
		s_Scene.Remove<FoliageComponent>(entity);
		s_EditedFoliageEntity = NoEntity;
		return;
	}

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.45f);
	ImGui::InputScalar("Seed##Foliage", ImGuiDataType_U32, &s.Seed);
	scatterEdited();
	FoliageTooltip("The scatter's number: the same seed, settings and terrain give the same places on every machine");
	ImGui::SameLine();
	if (ImGui::Button("Randomize##Foliage"))
	{
		s.Seed = Terrain::RandomSeed();
		commit = true;
	}
	FoliageTooltip("Other places: a new seed, the same rules");
	ImGui::DragFloat("Cell Size", &s.CellSize, 1.0f, 16.0f, 512.0f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	scatterEdited();
	FoliageTooltip("The copies are grouped in squares this wide: each square is culled on its own, and a species' copies in\n"
		"neighboring squares in view are one draw call. Smaller culls closer to the view, larger is fewer draw calls");
	ImGui::DragFloat("Clear Height", &s.ClearHeight, 1.0f, 0.0f, 1000.0f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	scatterEdited();
	FoliageTooltip("How high above the ground the keep-out looks for colliders: no copy under a road or a bridge this high\n"
		"(nor at the foot of its supports, which stand under it)");
	// The distances: drawing only, at once
	ImGui::DragFloat("Draw Distance", &foliage->Settings.DrawDistance, 5.0f, 0.0f, 20000.0f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	FoliageTooltip("Cells farther than this from the camera aren't drawn");
	ImGui::DragFloat("Shadow Distance", &foliage->Settings.ShadowDistance, 5.0f, 0.0f, foliage->Settings.DrawDistance, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	FoliageTooltip("Cells farther than this from the camera cast no shadow (the shadow maps are drawn for each light and\n"
		"cascade: far shadows cost as much as near ones and are hardly seen)");
	ImGui::DragFloat("Impostor Fade", &foliage->Settings.ImpostorFade, 0.5f, 0.0f, 1000.0f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	FoliageTooltip("Meters over which a copy goes from its model to its impostor, beyond its species' Impostor Distance: the\n"
		"two cross-fade, each pixel the model's or the card's (smoothed by the anti-aliasing). 0: a sudden switch");
	s.DrawDistance = foliage->Settings.DrawDistance;
	s.ShadowDistance = foliage->Settings.ShadowDistance;
	s.ImpostorFade = foliage->Settings.ImpostorFade;

	int removeSpecies = -1;
	for (size_t i = 0; i < s.Species.size(); i++)
	{
		FoliageSpecies& species = s.Species[i];
		ImGui::PushID((int)i);
		const std::string title = "Species " + std::to_string(i + 1) + ": " +
			(species.Model.empty() ? std::string("no model") : std::filesystem::u8path(species.Model).stem().u8string()) + "###Species";
		if (ImGui::TreeNodeEx(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			commit = SpeciesModelControl(species.Model, "##Model", "Model...", "(drop a model here)",
				"The model each copy is (static: no bones). Its meshes keep the materials the file gives them, from the\n"
				"Material Library: a leaves' material set to Cutout there cuts the leaves' cards in every pass.\n"
				"Drop a model from the Content Browser, or pick one") || commit;
			ImGui::DragFloat("Density", &species.Density, 0.5f, 0.0f, FoliageSpecies::MaxDensity, "%.1f per ha", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
			scatterEdited();
			FoliageTooltip("Copies per hectare (100 m x 100 m) where the rules let them stand: 10 a sparse wood, 100 a dense one,\n"
				"1000 a field of bushes. Spread evenly: a square of ground per copy, the copy at a random place in it");
			ImGui::DragFloatRange2("Scale", &species.MinScale, &species.MaxScale, 0.01f, 0.01f, 100.0f, "%.2f", "%.2f", ImGuiSliderFlags_AlwaysClamp);
			scatterEdited();
			FoliageTooltip("Each copy's size, times the model's: random between the two");
			ImGui::SliderFloat("Max Slope", &species.MaxSlope, 0.0f, 90.0f, "%.0f deg");
			scatterEdited();
			FoliageTooltip("No copies on ground steeper than this");
			ImGui::DragFloatRange2("Height", &species.MinHeight, &species.MaxHeight, 0.5f, -1.0e6f, 1.0e6f, "%.0f m", "%.0f m", ImGuiSliderFlags_AlwaysClamp);
			scatterEdited();
			FoliageTooltip("Only between these heights of the terrain (its own: 0 is its lowest point): not on the beach, not on the\n"
				"bare peaks");
			ImGui::DragFloat("Keep Out", &species.KeepOut, 0.1f, 0.0f, 500.0f, "%.1f m", ImGuiSliderFlags_AlwaysClamp);
			scatterEdited();
			FoliageTooltip("No copies this close to a collider other than the terrain (a road, its walls, a house), nor under one up\n"
				"to Clear Height: a tree's crown stays off the road if this is at least its radius");
			ImGui::DragFloat("Max Distance", &species.MaxDistance, 1.0f, 0.0f, 5000.0f, species.MaxDistance > 0.0f ? "%.0f m" : "anywhere",
				ImGuiSliderFlags_AlwaysClamp);
			scatterEdited();
			FoliageTooltip("Copies only this close to such a collider (0: anywhere): along a track, where its driver sees them, and\n"
				"none on the far sides of the hills");
			ImGui::SliderFloat("Align To Slope", &species.AlignToSlope, 0.0f, 1.0f, "%.2f");
			scatterEdited();
			FoliageTooltip("0: upright (trees grow up whatever the slope); 1: tilted with the ground (bushes, rocks)");
			ImGui::DragFloat("Sink", &species.Sink, 0.01f, 0.0f, 10.0f, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
			scatterEdited();
			FoliageTooltip("How deep each copy stands in the ground: no gap under its downhill side on a slope");
			// The impostor: drawing only, at once (not a scatter setting): the component's own species, kept the same in the edited copy
			FoliageSpecies& drawn = i < foliage->Settings.Species.size() ? foliage->Settings.Species[i] : species;
			if (SpeciesModelControl(drawn.Impostor, "##Impostor", "Impostor...", "(no impostor: the model at any distance)",
				"Far from the camera, a copy is drawn as this: a card turned to face the camera, showing the model as seen\n"
				"from the nearest of the directions it was baked from (Tools/TrackBuilder/bake_impostors.py writes\n"
				"<model>_Impostor.glb beside the model: a card with atlases of 16 views). Its shadow is still the model's"))
			{
				species.Impostor = drawn.Impostor;
			}
			if (!drawn.Impostor.empty())
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("X##Impostor"))
				{
					drawn.Impostor.clear();
					species.Impostor.clear();
				}
				FoliageTooltip("No impostor: the model at any distance");
			}
			ImGui::DragFloat("Impostor Distance", &drawn.ImpostorDistance, 1.0f, 0.0f, 5000.0f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
			FoliageTooltip("Copies farther than this from the camera are drawn as the impostor (with one), after the foliage's\n"
				"Impostor Fade");
			species.ImpostorDistance = drawn.ImpostorDistance;
			if (ImGui::Button("Remove Species"))
			{
				removeSpecies = (int)i;
			}
			if (foliage->Instances && i < foliage->Instances->CopiesBySpecies.size())
			{
				ImGui::SameLine();
				ImGui::TextDisabled("%u copies", foliage->Instances->CopiesBySpecies[i]);
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	if (removeSpecies >= 0)
	{
		s.Species.erase(s.Species.begin() + removeSpecies);
		commit = true;
	}
	if (ImGui::Button("Add Species"))
	{
		s.Species.push_back(FoliageSpecies());
		commit = true;
	}

	if (commit)
	{
		s.Clamp();
		foliage->Settings = s;
	}

	if (foliage->Instances)
	{
		const FoliageInstances& instances = *foliage->Instances;
		ImGui::Spacing();
		ImGui::TextDisabled("%zu copies in %zu cells, %u places kept out; scattered in %.0f ms", instances.Copies.size(), instances.Cells.size(),
			instances.KeptOut, instances.Milliseconds);
	}
}
