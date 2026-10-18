// The Material Library and Material Editor panels, and the material changes they ask for


#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkan/EnvMapVulkanProfiler.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"
#include "EnvMapVulkan/EnvMapVulkanSceneSerializer.h"

#include "Core/Application.h"
#include "Core/FileDialogs.h"
#include "Core/Input.h"
#include "Core/ResourceManager.h"
#include "Core/Util.h"
#include "H2M/Core/Math/RayH2M.h"
#include "ImGui/ImGuiWrapper.h"
#include "Scene/Scene.h"
#include "H2M/Editor/ContentBrowserPanelH2M.h"

#include "imgui.h"
#include "imgui_internal.h" // BeginDragDropTargetCustom (the Models and Meshes panel as one drop area)
#include "ImGuizmo.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>


#include "EditorState.h"


using namespace EnvMapVulkanSceneData;

static bool IsMaterialFile(const std::string& filepath)
{
	return std::filesystem::path(filepath).extension() == EnvMapVulkanMaterialLibrary::FileExtension;
}

bool IsImageFile(const std::string& filepath)
{
	static const std::set<std::string> s_Extensions = { ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".psd", ".gif" };
	std::string extension = std::filesystem::path(filepath).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return s_Extensions.find(extension) != s_Extensions.end();
}

// Number of meshes, over all loaded models, drawn with the material
static int CountMaterialUsers(const H2M::RefH2M<EnvMapVulkanMaterial>& material)
{
	int count = 0;
	s_Scene.Each<MeshPartComponent>([&](EnvMapVulkanEntityID, MeshPartComponent& part) {
		count += part.Material == material ? 1 : 0;
	});
	return count;
}

// The Material Editor follows the selection: selecting a mesh (Models and Meshes panel or viewport) selects the material it is drawn with
static void SyncSelectedMaterial()
{
	static EnvMapVulkanEntityID s_LastSelected = NoEntity;
	if (s_EditorSelection.GetPrimary() == s_LastSelected)
	{
		return;
	}
	s_LastSelected = s_EditorSelection.GetPrimary();

	if (const MeshPartComponent* part = s_Scene.TryGet<MeshPartComponent>(s_EditorSelection.GetPrimary()))
	{
		s_SelectedMaterial = part->Material;
	}
}

// Loads an image and binds it as one of a material's maps, or removes the map when the request has no file path
// (called at the start of a frame, see EditorPreFrame)
void ApplyMaterialTexture(const PendingMaterialTexture& request)
{
	H2M::RefH2M<EnvMapVulkanMaterial> material = request.Material;
	if (!material || request.Slot >= EnvMapVulkanMaterial::MapCount)
	{
		return;
	}

	if (request.FilePath.empty())
	{
		EnvMapVulkanRenderer::WaitIdle(); // the descriptor set may be used by frames in flight
		material->RemoveMap(request.Slot);
		Log::GetLogger()->info("{0} removed from material '{1}'", EnvMapVulkanMaterial::GetMapTextureName(request.Slot), material->GetName());
		return;
	}

	if (!std::filesystem::exists(request.FilePath) || !IsImageFile(request.FilePath))
	{
		Log::GetLogger()->error("Map '{0}' was not loaded: the file does not exist or is not a supported image.", request.FilePath);
		return;
	}

	// Through the texture cache: an image already loaded (by a model or another material) in this slot's color space is shared
	H2M::RefH2M<H2M::Texture2D_H2M> texture = ResourceManager::LoadTexture2D_H2M(request.FilePath, EnvMapVulkanMaterial::IsColorMap(request.Slot));
	if (!texture || !texture->Loaded())
	{
		Log::GetLogger()->error("Map '{0}' could not be loaded.", request.FilePath);
		return;
	}

	EnvMapVulkanRenderer::WaitIdle(); // the descriptor set may be used by frames in flight
	material->SetMap(request.Slot, texture);

	Log::GetLogger()->info("Map '{0}' assigned to {1} of material '{2}'", request.FilePath, EnvMapVulkanMaterial::GetMapTextureName(request.Slot), material->GetName());
}

// Deletes a material from the library (called at the start of a frame, see EditorPreFrame). Meshes that used it get the Default material.
void DeleteMaterial(H2M::RefH2M<EnvMapVulkanMaterial> material)
{
	EnvMapVulkanRenderer::WaitIdle(); // its descriptor set may be used by frames in flight

	int users = CountMaterialUsers(material);
	EnvMapVulkanMaterialLibrary::Remove(material);
	if (users > 0)
	{
		H2M::RefH2M<EnvMapVulkanMaterial> replacement = EnvMapVulkanMaterialLibrary::GetDefaultMaterial(); // a new one if the Default was deleted
		s_Scene.Each<MeshPartComponent>([&](EnvMapVulkanEntityID, MeshPartComponent& part) {
			if (part.Material == material)
			{
				part.Material = replacement;
			}
		});
	}
	if (s_SelectedMaterial == material)
	{
		s_SelectedMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
	}
	Log::GetLogger()->info("Material '{0}' deleted ({1} meshes now use the Default material)", material->GetName(), users);
}

// "Make Unique": the part gets a new variant of its material. It looks the same and follows the material until one of its
// properties is edited, which then applies to this part only (and to other parts given the variant).
void MakePartMaterialUnique(EnvMapVulkanEntityID partEntity)
{
	MeshPartComponent* part = s_Scene.TryGet<MeshPartComponent>(partEntity);
	if (!part)
	{
		return;
	}
	H2M::RefH2M<EnvMapVulkanMaterial> parent = part->Material ? part->Material : EnvMapVulkanMaterialLibrary::GetDefaultMaterial();
	part->Material = EnvMapVulkanMaterialLibrary::CreateVariant(parent); // a new descriptor set: no frame in flight uses it
	s_SelectedMaterial = part->Material;
	Log::GetLogger()->info("Part '{0}': its material is now '{1}', a variant of '{2}'",
		s_Scene.Get<NameComponent>(partEntity).Name, part->Material->GetName(), parent->GetName());
}

// All materials of the scene: create, duplicate, delete, and drag onto a mesh (Models and Meshes panel or viewport) to assign
void OnImGuiRenderMaterialLibrary()
{
	ImGui::Begin("Material Library");

	SyncSelectedMaterial();

	// Renaming in place (double-click, F2, the Rename button or the context menu): the row turns into a text field.
	// Enter or clicking elsewhere applies the name, Escape cancels.
	static EnvMapVulkanMaterial* s_RenamingMaterial = nullptr;
	static bool s_RenameNeedsFocus = false;
	static char s_RenameBuffer[128] = "";
	auto startRename = [](const H2M::RefH2M<EnvMapVulkanMaterial>& material) {
		s_RenamingMaterial = const_cast<EnvMapVulkanMaterial*>(material.Raw());
		s_RenameNeedsFocus = true;
		snprintf(s_RenameBuffer, sizeof(s_RenameBuffer), "%s", material->GetName().c_str());
	};

	// Duplicating adds to the material list, so a request from a row is carried out after the list is drawn
	H2M::RefH2M<EnvMapVulkanMaterial> duplicateRequest = H2M::RefH2M<EnvMapVulkanMaterial>();
	H2M::RefH2M<EnvMapVulkanMaterial> variantRequest = H2M::RefH2M<EnvMapVulkanMaterial>();

	if (ImGui::Button("New Material"))
	{
		s_SelectedMaterial = EnvMapVulkanMaterialLibrary::CreateMaterial();
		startRename(s_SelectedMaterial); // a new material usually gets a name right away
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!s_SelectedMaterial);
	if (ImGui::Button("Rename"))
	{
		startRename(s_SelectedMaterial);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Rename the selected material (or double-click it, or press F2)");
	}
	ImGui::SameLine();
	if (ImGui::Button("Duplicate"))
	{
		duplicateRequest = s_SelectedMaterial;
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("An independent copy of the selected material, with the same values and maps");
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete"))
	{
		s_PendingDeleteMaterial = s_SelectedMaterial; // deleted at the start of the next frame (see EditorPreFrame)
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Meshes that use the selected material get the Default material");
	}
	ImGui::EndDisabled();

	// Material files (.mmat): one file per material. The result of a save shows below the buttons for a few seconds.
	static std::string s_SaveStatus;
	static double s_SaveStatusTime = -100.0;
	auto setStatus = [](const std::string& status) {
		s_SaveStatus = status;
		s_SaveStatusTime = ImGui::GetTime();
	};
	auto saveOne = [&setStatus](H2M::RefH2M<EnvMapVulkanMaterial> material, const std::string& filepath) {
		if (EnvMapVulkanMaterialLibrary::Save(material, filepath))
		{
			setStatus("Saved '" + material->GetName() + "' to " + material->GetFilePath());
		}
		else
		{
			setStatus("'" + material->GetName() + "' could not be saved (see the log)");
		}
	};
	ImGui::BeginDisabled(!s_SelectedMaterial);
	if (ImGui::Button("Save"))
	{
		saveOne(s_SelectedMaterial, "");
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Saves the selected material to its file, or (never saved) to %s/<name>%s\n"
			"(file names have no spaces: \"My Material\" is saved as My_Material%s)",
			EnvMapVulkanMaterialLibrary::MaterialsFolder, EnvMapVulkanMaterialLibrary::FileExtension, EnvMapVulkanMaterialLibrary::FileExtension);
	}
	ImGui::SameLine();
	if (ImGui::Button("Save As..."))
	{
		std::string filepath = FileDialogs::SaveFile({ { "Material", "mmat" } });
		if (!filepath.empty())
		{
			saveOne(s_SelectedMaterial, filepath);
		}
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Save All"))
	{
		uint32_t saved = 0, failed = 0;
		for (const auto& material : EnvMapVulkanMaterialLibrary::GetMaterials())
		{
			if (material->HasUnsavedChanges())
			{
				EnvMapVulkanMaterialLibrary::Save(material) ? saved++ : failed++;
			}
		}
		Log::GetLogger()->info("Save All: {0} material(s) saved, {1} failed", saved, failed);
		if (saved == 0 && failed == 0)
		{
			setStatus("All materials are already saved (nothing marked *)");
		}
		else
		{
			setStatus("Saved " + std::to_string(saved) + " material" + (saved == 1 ? "" : "s") + " (one file each)" +
				(failed > 0 ? ", " + std::to_string(failed) + " could not be saved (see the log)" : ""));
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Saves every material with unsaved changes (marked *), each to its own file;\n"
			"one never saved goes to %s", EnvMapVulkanMaterialLibrary::MaterialsFolder);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load..."))
	{
		std::string filepath = FileDialogs::OpenFile({ { "Material", "mmat" } });
		if (!filepath.empty())
		{
			s_PendingMaterialFiles.push_back(filepath); // loaded at the start of the next frame (see EditorPreFrame)
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Loads a material file (or drop .mmat files onto this panel). A file of a material already in the\n"
			"library (the same ID) reloads it; the others are added. Saved materials are in %s.", EnvMapVulkanMaterialLibrary::MaterialsFolder);
	}

	if (ImGui::GetTime() - s_SaveStatusTime < 5.0)
	{
		ImGui::TextDisabled("%s", s_SaveStatus.c_str());
	}

	static char s_Filter[64] = "";
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##filter", "Filter by name", s_Filter, sizeof(s_Filter));

	ImGui::Separator();

	std::string filter = s_Filter;
	std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char c) { return (char)std::tolower(c); });

	const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();
	if (materials.empty())
	{
		ImGui::TextDisabled("No materials. Load a model or click New Material.");
	}

	const float countColumnWidth = ImGui::CalcTextSize("000 uses").x;
	for (uint32_t m = 0; m < (uint32_t)materials.size(); m++)
	{
		const H2M::RefH2M<EnvMapVulkanMaterial>& material = materials[m];

		std::string lowerName = material->GetName();
		std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		if (!filter.empty() && lowerName.find(filter) == std::string::npos)
		{
			continue;
		}

		ImGui::PushID((int)m);
		ImVec2 rowSize(ImGui::GetContentRegionAvail().x - countColumnWidth, 0.0f);

		if (s_RenamingMaterial == material.Raw())
		{
			ImGui::SetNextItemWidth(rowSize.x);
			if (s_RenameNeedsFocus)
			{
				ImGui::SetKeyboardFocusHere();
				s_RenameNeedsFocus = false;
			}
			bool entered = ImGui::InputText("##rename", s_RenameBuffer, sizeof(s_RenameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
			if (entered || (ImGui::IsItemDeactivated() && !ImGui::IsKeyPressed(ImGuiKey_Escape)))
			{
				EnvMapVulkanMaterialLibrary::Rename(material, s_RenameBuffer);
				s_RenamingMaterial = nullptr;
			}
			else if (ImGui::IsItemDeactivated())
			{
				s_RenamingMaterial = nullptr; // Escape: keep the old name
			}
		}
		else if (ImGui::Selectable((material->GetName() + (material->HasUnsavedChanges() ? " *" : "") + "##material").c_str(),
			s_SelectedMaterial == material, ImGuiSelectableFlags_AllowDoubleClick, rowSize))
		{
			s_SelectedMaterial = material;
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				startRename(material);
			}
		}

		if (ImGui::BeginPopupContextItem("##materialContextMenu"))
		{
			s_SelectedMaterial = material;
			if (ImGui::MenuItem("Rename", "F2"))
			{
				startRename(material);
			}
			if (ImGui::MenuItem("Duplicate"))
			{
				duplicateRequest = material;
			}
			if (ImGui::MenuItem("Create Variant"))
			{
				variantRequest = material;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("A new material that inherits everything from this one (including later changes)\n"
					"until you override some of its properties in the Material Editor");
			}
			if (ImGui::MenuItem("Delete"))
			{
				s_PendingDeleteMaterial = material; // deleted at the start of the next frame (see EditorPreFrame)
			}
			ImGui::EndPopup();
		}

		if (s_RenamingMaterial != material.Raw() && ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(s_MaterialPayload, &m, sizeof(uint32_t));
			ImGui::Text("%s", material->GetName().c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::IsItemHovered())
		{
			std::string origin = material->GetSourceFile().empty() ? "Created in the editor" : "Imported from " + material->GetSourceFile();
			if (material->IsVariant())
			{
				H2M::RefH2M<EnvMapVulkanMaterial> parent = EnvMapVulkanMaterialLibrary::GetParent(material);
				origin = "Variant of " + (parent ? "'" + parent->GetName() + "'" : std::string("a material that isn't loaded")) +
					", " + std::to_string(material->GetOverrideCount()) + " overridden properties";
			}
			std::string file = material->GetFilePath().empty() ? "Not saved yet (*)" :
				"File: " + material->GetFilePath() + (material->HasUnsavedChanges() ? " (unsaved changes *)" : "");
			ImGui::SetTooltip("%s\n%s\nDrag onto a mesh in the Scene Hierarchy, the Properties panel or the viewport to assign it",
				origin.c_str(), file.c_str());
		}

		int users = CountMaterialUsers(material);
		ImGui::SameLine();
		if (users > 0)
		{
			ImGui::TextDisabled("%d %s", users, users == 1 ? "use" : "uses");
		}
		else
		{
			ImGui::TextDisabled("unused");
		}
		ImGui::PopID();
	}

	// F2 renames the selected material while this panel has focus (and no text field is being edited)
	if (s_SelectedMaterial && !s_RenamingMaterial && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
		!ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_F2, false))
	{
		startRename(s_SelectedMaterial);
	}

	if (duplicateRequest)
	{
		s_SelectedMaterial = EnvMapVulkanMaterialLibrary::Duplicate(duplicateRequest);
	}
	if (variantRequest)
	{
		s_SelectedMaterial = EnvMapVulkanMaterialLibrary::CreateVariant(variantRequest);
	}

	// The whole panel is a drop area for material files
	ImVec2 panelMin = ImGui::GetWindowPos();
	ImVec2 panelMax(panelMin.x + ImGui::GetWindowSize().x, panelMin.y + ImGui::GetWindowSize().y);
	if (ImGui::BeginDragDropTargetCustom(ImRect(panelMin, panelMax), ImGui::GetID("##MaterialLibraryDropArea")))
	{
		std::string filepath;
		if (AcceptFileDrop(panelMin, panelMax, IsMaterialFile, "a material file (.mmat)", filepath))
		{
			s_PendingMaterialFiles.push_back(filepath); // loaded at the start of the next frame (see EditorPreFrame)
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::End();
}

// Edits the selected library material in place: every mesh drawn with it changes (values are push constants of every
// draw; maps are written to the material's descriptor set at the start of the next frame)
void OnImGuiRenderMaterialEditor()
{
	ImGui::Begin("Material Editor");

	SyncSelectedMaterial();

	H2M::RefH2M<EnvMapVulkanMaterial> material = s_SelectedMaterial;
	if (!material)
	{
		ImGui::TextDisabled("Select a material in the Material Library,\nor a mesh in the Scene Hierarchy or the viewport");
		ImGui::End();
		return;
	}

	// Name (renamed when editing ends; names stay unique in the library)
	static char s_NameBuffer[128] = "";
	static EnvMapVulkanMaterial* s_NameBufferMaterial = nullptr;
	if (s_NameBufferMaterial != material.Raw() || !ImGui::IsAnyItemActive())
	{
		snprintf(s_NameBuffer, sizeof(s_NameBuffer), "%s", material->GetName().c_str());
		s_NameBufferMaterial = material.Raw();
	}
	ImGui::InputText("Name", s_NameBuffer, sizeof(s_NameBuffer));
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		EnvMapVulkanMaterialLibrary::Rename(material, s_NameBuffer);
	}

	int users = CountMaterialUsers(material);
	if (users > 1)
	{
		ImGui::TextDisabled("Used by %d meshes: changes apply to all of them.", users);
	}
	else if (users == 0)
	{
		ImGui::TextDisabled("Not used by any mesh.");
	}

	// The selected mesh's own material: a variant of this one (see MakePartMaterialUnique)
	const MeshPartComponent* selectedPart = s_Scene.TryGet<MeshPartComponent>(s_EditorSelection.GetPrimary());
	if (selectedPart && selectedPart->Material == material)
	{
		if (ImGui::Button("Make Unique"))
		{
			MakePartMaterialUnique(s_EditorSelection.GetPrimary());
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Gives the selected mesh ('%s') a variant of this material: it looks the same and follows this\n"
				"material's changes, but the properties you edit then apply to that mesh only",
				s_Scene.Get<NameComponent>(s_EditorSelection.GetPrimary()).Name.c_str());
		}
	}

	// A variant: its parent, and which properties it overrides. Inherited properties are grayed out (editing one overrides
	// it); overridden ones have a Reset button that makes them inherited again.
	const bool variant = material->IsVariant();
	H2M::RefH2M<EnvMapVulkanMaterial> parent = EnvMapVulkanMaterialLibrary::GetParent(material);
	if (variant)
	{
		ImGui::Separator();
		if (parent)
		{
			ImGui::TextDisabled("Variant of");
			ImGui::SameLine();
			if (ImGui::SmallButton(parent->GetName().c_str()))
			{
				s_SelectedMaterial = parent;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Edit the parent material (its changes apply to this variant's inherited properties)");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Variant of a material that isn't loaded: it keeps its values");
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Make Independent"))
		{
			EnvMapVulkanMaterialLibrary::Detach(material);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("This material keeps its current values but no longer follows its parent");
		}
		const uint32_t overrides = material->GetOverrideCount();
		ImGui::TextDisabled("%u overridden propert%s. Grayed ones are inherited: edit one to override it.", overrides, overrides == 1 ? "y" : "ies");
	}

	// Around each property's controls: grayed while inherited, a Reset button when overridden
	auto beginProperty = [&](EnvMapVulkanMaterial::Property property) {
		const bool inherited = variant && !material->IsOverridden(property);
		if (inherited)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		return inherited;
	};
	auto endProperty = [&](EnvMapVulkanMaterial::Property property, bool inherited) {
		if (inherited)
		{
			ImGui::PopStyleVar();
		}
		else if (variant)
		{
			ImGui::SameLine();
			ImGui::PushID((int)property);
			if (ImGui::SmallButton("Reset"))
			{
				EnvMapVulkanMaterialLibrary::ResetOverride(material, property); // the parent's value from the next frame
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Overridden. Reset: inherit it from '%s' again", parent ? parent->GetName().c_str() : "the parent");
			}
			ImGui::PopID();
		}
	};

	ImGui::Separator();

	// The surface: opaque (PBR) or glass, and the glass values
	bool inherited = beginProperty(EnvMapVulkanMaterial::SurfaceProperty);
	int surface = material->IsGlass() ? 1 : 0;
	if (ImGui::Combo("Surface", &surface, "Opaque\0Glass\0"))
	{
		material->SetSurface(surface == 1 ? EnvMapVulkanMaterial::Surface::Glass : EnvMapVulkanMaterial::Surface::Opaque);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Opaque: light is reflected or scattered back, nothing passes through.\n"
			"Glass: light passes through it, bent by its index of refraction. The albedo color (or map) tints it,\n"
			"the roughness frosts it, the normal map ripples it. Metalness and AO don't apply.");
	}
	endProperty(EnvMapVulkanMaterial::SurfaceProperty, inherited);
	const bool glass = material->IsGlass();
	if (glass)
	{
		float& ior = material->GetGlassValue(EnvMapVulkanMaterial::GlassIOR);
		float& thickness = material->GetGlassValue(EnvMapVulkanMaterial::GlassThickness);
		float& castShadows = material->GetGlassValue(EnvMapVulkanMaterial::GlassCastShadows);
		float& solid = material->GetGlassValue(EnvMapVulkanMaterial::GlassSolid);

		ImGui::Indent();
		inherited = beginProperty(EnvMapVulkanMaterial::GetGlassProperty(EnvMapVulkanMaterial::GlassSolid));
		int shape = solid > 0.5f ? 0 : 1;
		if (ImGui::Combo("Shape", &shape, "Solid\0Thin\0"))
		{
			solid = shape == 0 ? 1.0f : 0.0f;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Solid: a block, a ball, a bottle full of liquid. The light bends entering and leaving, as through\n"
				"a lens: what's behind it is squeezed and turned around, more toward its outline.\n"
				"Thin: a pane, a shell, a hollow vase. What's behind only shifts a little.");
		}
		endProperty(EnvMapVulkanMaterial::GetGlassProperty(EnvMapVulkanMaterial::GlassSolid), inherited);
		inherited = beginProperty(EnvMapVulkanMaterial::GetGlassProperty(EnvMapVulkanMaterial::GlassIOR));
		ImGui::SliderFloat("Index of Refraction", &ior, 1.0f, 2.5f, "%.2f");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("How much the light bends entering it, and how much it reflects head-on.\n"
				"1.00 air (no bending), 1.33 water, 1.50 glass, 1.77 sapphire, 2.42 diamond");
		}
		endProperty(EnvMapVulkanMaterial::GetGlassProperty(EnvMapVulkanMaterial::GlassIOR), inherited);
		inherited = beginProperty(EnvMapVulkanMaterial::GetGlassProperty(EnvMapVulkanMaterial::GlassThickness));
		ImGui::DragFloat("Thickness", &thickness, 0.005f, 0.0f, 10.0f, "%.3f m");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("How far the light travels inside the glass: how far what's behind it seems to move.\n"
				"A window pane: about 0.01 m; a glass block or a vase: its size");
		}
		endProperty(EnvMapVulkanMaterial::GetGlassProperty(EnvMapVulkanMaterial::GlassThickness), inherited);
		inherited = beginProperty(EnvMapVulkanMaterial::GetGlassProperty(EnvMapVulkanMaterial::GlassCastShadows));
		bool shadows = castShadows > 0.5f;
		if (ImGui::Checkbox("Cast Shadows", &shadows))
		{
			castShadows = shadows ? 1.0f : 0.0f;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Off: the light passes through (no shadow). On: a full shadow, as an opaque mesh casts");
		}
		endProperty(EnvMapVulkanMaterial::GetGlassProperty(EnvMapVulkanMaterial::GlassCastShadows), inherited);
		ImGui::Unindent();
		ImGui::Separator();
	}

	glm::vec3& albedoColor = material->Get<glm::vec3>("u_MaterialUniforms.AlbedoColor");
	float& metalness = material->Get<float>("u_MaterialUniforms.Metalness");
	float& roughness = material->Get<float>("u_MaterialUniforms.Roughness");

	inherited = beginProperty(EnvMapVulkanMaterial::AlbedoColorProperty);
	ImGui::ColorEdit3(glass ? "Tint Color" : "Albedo Color", &albedoColor.x);
	if (glass && ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The albedo color: for glass, the color of the light that passes through it (white: clear)");
	}
	endProperty(EnvMapVulkanMaterial::AlbedoColorProperty, inherited);
	inherited = beginProperty(EnvMapVulkanMaterial::MetalnessProperty);
	ImGui::BeginDisabled(glass);
	ImGui::SliderFloat("Metalness", &metalness, 0.0f, 1.0f);
	ImGui::EndDisabled();
	endProperty(EnvMapVulkanMaterial::MetalnessProperty, inherited);
	inherited = beginProperty(EnvMapVulkanMaterial::RoughnessProperty);
	ImGui::SliderFloat(glass ? "Roughness (Frost)" : "Roughness", &roughness, 0.0f, 1.0f);
	endProperty(EnvMapVulkanMaterial::RoughnessProperty, inherited);

	float& tilingFactor = material->Get<float>("u_MaterialUniforms.TilingFactor");
	float& emissiveIntensity = material->Get<float>("u_MaterialUniforms.EmissiveIntensity");
	float& metalRoughPacked = material->Get<float>("u_MaterialUniforms.MetalRoughPacked");

	inherited = beginProperty(EnvMapVulkanMaterial::TilingFactorProperty);
	ImGui::DragFloat("Tiling Factor", &tilingFactor, 0.01f, 0.01f, 100.0f);
	endProperty(EnvMapVulkanMaterial::TilingFactorProperty, inherited);
	inherited = beginProperty(EnvMapVulkanMaterial::EmissiveIntensityProperty);
	ImGui::DragFloat("Emissive Intensity", &emissiveIntensity, 0.05f, 0.0f, 100.0f);
	endProperty(EnvMapVulkanMaterial::EmissiveIntensityProperty, inherited);

	inherited = beginProperty(EnvMapVulkanMaterial::MetalRoughPackedProperty);
	bool packed = metalRoughPacked > 0.5f;
	if (ImGui::Checkbox("Packed Metalness/Roughness Map", &packed))
	{
		metalRoughPacked = packed ? 1.0f : 0.0f;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("glTF metallicRoughness map: roughness is read from G and metalness from B (otherwise both from R).\n"
			"Assign the same map to both the metalness and the roughness slot.");
	}
	endProperty(EnvMapVulkanMaterial::MetalRoughPackedProperty, inherited);

	static const char* s_MapLabels[EnvMapVulkanMaterial::MapCount] = {
		"Use Albedo Map", "Use Normal Map", "Use Metalness Map", "Use Roughness Map", "Use Emissive Map", "Use AO Map" };

	for (uint32_t i = 0; i < EnvMapVulkanMaterial::MapCount; i++)
	{
		ImGui::PushID((int)i);
		ImGui::Separator();

		const bool mapInherited = beginProperty(EnvMapVulkanMaterial::GetMapProperty(i));
		H2M::RefH2M<H2M::Texture2D_H2M> map = material->GetMap(i);

		// Thumbnail of the map; also a drop target for images from the Content Browser
		const ImVec2 thumbnailSize(64.0f, 64.0f);
		ImTextureID thumbnail = map ? map->GetImTextureID() : ImTextureID{};
		if (thumbnail)
		{
			ImGui::Image(thumbnail, thumbnailSize, ImVec2(0, 1), ImVec2(1, 0)); // the Vulkan loader flips LDR images
		}
		else
		{
			ImGui::Button(map ? "map" : "no map", thumbnailSize);
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
				if (IsImageFile(itemPath))
				{
					s_PendingMaterialTextures.push_back({ material, i, itemPath }); // applied at the start of the next frame
				}
				else
				{
					Log::GetLogger()->warn("'{0}' is not a supported image file", itemPath);
				}
			}
			ImGui::EndDragDropTarget();
		}
		if (ImGui::IsItemHovered())
		{
			const char* colorSpace = EnvMapVulkanMaterial::IsColorMap(i) ? "sRGB (color data)" : "linear (raw data)";
			if (map)
			{
				ImGui::SetTooltip("%s\n%s", map->GetPath().c_str(), colorSpace);
			}
			else
			{
				ImGui::SetTooltip("Drop an image from the Content Browser here\nLoaded as %s", colorSpace);
			}
		}

		ImGui::SameLine();
		ImGui::BeginGroup();
		{
			float& toggle = material->Get<float>(EnvMapVulkanMaterial::GetMapToggleName(i));
			bool enabled = toggle > 0.5f;

			ImGui::BeginDisabled(!map);
			if (ImGui::Checkbox(s_MapLabels[i], &enabled))
			{
				toggle = enabled ? 1.0f : 0.0f;
			}
			ImGui::EndDisabled();

			if (ImGui::Button("Load..."))
			{
				std::string filepath = FileDialogs::OpenFile({ { "Images", "png,jpg,jpeg,tga,bmp" } });
				if (!filepath.empty())
				{
					s_PendingMaterialTextures.push_back({ material, i, filepath }); // applied at the start of the next frame
				}
			}

			// Remove the map; the material then uses its value
			if (map)
			{
				ImGui::SameLine();
				if (ImGui::Button("Remove"))
				{
					s_PendingMaterialTextures.push_back({ material, i, std::string() }); // applied at the start of the next frame
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Remove this map: the material uses its value instead\n(the model file is not changed)");
				}
				ImGui::SameLine();
				ImGui::TextDisabled("%s", std::filesystem::path(map->GetPath()).filename().string().c_str());
			}
		}
		ImGui::EndGroup();
		endProperty(EnvMapVulkanMaterial::GetMapProperty(i), mapInherited);

		ImGui::PopID();
	}

	ImGui::End();
}
