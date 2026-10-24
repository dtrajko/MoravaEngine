// The Scene Hierarchy panel


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

static const char* s_EntityPayload = "VULKAN_SCENE_ENTITY"; // drag & drop payload: an EnvMapVulkanEntityID
static bool s_RevealSelection = false; // this frame: open the selected entity's parents in the tree and scroll to it

static void StartRenaming(EnvMapVulkanEntityID entity)
{
	s_RenamingEntity = entity;
	snprintf(s_RenameBuffer, sizeof(s_RenameBuffer), "%s", s_Scene.Get<NameComponent>(entity).Name.c_str());
}

// A material dropped onto a part (that part) or a model (all its parts). Dropped onto one of several selected entities,
// it goes to all of them (every selected part, and every part of a selected model), and the selection stays.
void AssignMaterialToDropTarget(EnvMapVulkanEntityID target, H2M::RefH2M<EnvMapVulkanMaterial> material)
{
	const bool toSelection = s_EditorSelection.IsSelected(target) && s_EditorSelection.GetAll().size() > 1;
	const std::vector<EnvMapVulkanEntityID> targets = toSelection ? s_EditorSelection.GetAll() : std::vector<EnvMapVulkanEntityID>{ target };
	for (EnvMapVulkanEntityID entity : targets)
	{
		if (MeshPartComponent* part = s_Scene.TryGet<MeshPartComponent>(entity))
		{
			part->Material = material;
		}
		else if (s_Scene.Has<ModelComponent>(entity))
		{
			for (EnvMapVulkanEntityID child : s_Scene.GetChildren(entity))
			{
				if (MeshPartComponent* childPart = s_Scene.TryGet<MeshPartComponent>(child))
				{
					childPart->Material = material;
				}
			}
		}
	}
	if (toSelection)
	{
		s_EditorSelection.SetPrimary(target);
	}
	else
	{
		s_EditorSelection.SelectOnly(target);
	}
	s_SelectedMaterial = material;
}

// A material dropped onto a part (that part) or a model (all its parts)
static void AcceptMaterialDropOnEntity(EnvMapVulkanEntityID entity)
{
	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_MaterialPayload);
	if (!payload)
	{
		return;
	}
	const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();
	uint32_t index = *(const uint32_t*)payload->Data;
	if (index >= materials.size())
	{
		return;
	}
	if (!s_Scene.Has<MeshPartComponent>(entity) && !s_Scene.Has<ModelComponent>(entity))
	{
		return;
	}
	AssignMaterialToDropTarget(entity, materials[index]);
}

// Deletes every selected entity that can be deleted (the sun and the environment stay). Lights and the water go right
// away, models and parts at the start of the next frame (see DeleteEntity).
void DeleteSelectedEntities()
{
	const std::vector<EnvMapVulkanEntityID> selection = s_EditorSelection.GetAll(); // a copy: deleting changes the selection
	for (EnvMapVulkanEntityID entity : selection)
	{
		if (s_Scene.Exists(entity) && CanBeDeleted(GetEntityKind(entity)))
		{
			DeleteEntity(entity);
		}
	}
}

// What the tree asks for while it's drawn (applied after it, so the tree isn't changed while it's walked)
struct HierarchyRequests
{
	bool DeleteSelection = false; // every selected entity that can be deleted
	EnvMapVulkanEntityID Reparent = NoEntity;
	EnvMapVulkanEntityID NewParent = NoEntity;
};

// The rows of the tree as drawn (open nodes only), top to bottom: last frame's, for Shift + click ranges, and this frame's
static std::vector<EnvMapVulkanEntityID> s_HierarchyRows;
static std::vector<EnvMapVulkanEntityID> s_HierarchyRowsDrawing;

// Shift + click: the rows from the anchor (the last plain or Ctrl click) to the clicked one; Ctrl + Shift adds them
static void SelectHierarchyRange(EnvMapVulkanEntityID entity, bool add)
{
	s_EditorSelection.SelectRange(s_HierarchyRows, entity, add);
}

static void DrawHierarchyNode(EnvMapVulkanEntityID entity, HierarchyRequests& requests)
{
	const EntityKind kind = GetEntityKind(entity);
	const std::string& name = s_Scene.Get<NameComponent>(entity).Name;
	const std::vector<EnvMapVulkanEntityID>& children = s_Scene.GetChildren(entity);
	const bool selected = s_EditorSelection.IsSelected(entity);
	s_HierarchyRowsDrawing.push_back(entity);

	ImGui::PushID((void*)(uintptr_t)entity);
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
	if (selected)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (children.empty())
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	if (kind != EntityKind::Model)
	{
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	// Room for the marker in front of the name ("##" keeps the ID stable while the name changes)
	const std::string label = "    " + (s_RenamingEntity == entity ? std::string() : name) + "##node";
	if (s_RevealSelection && s_Scene.IsDescendantOf(s_EditorSelection.GetPrimary(), entity))
	{
		ImGui::SetNextItemOpen(true); // a parent of the selection (e.g. a model whose part was clicked in the viewport)
	}
	const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
	if (s_RevealSelection && entity == s_EditorSelection.GetPrimary())
	{
		ImGui::SetScrollHereY(0.5f);
	}
	const ImVec2 itemMin = ImGui::GetItemRectMin();
	const ImVec2 itemMax = ImGui::GetItemRectMax();

	// Click: the entity alone; Ctrl + click adds or removes it; Shift + click selects the rows from the last clicked one
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
	{
		const ImGuiIO& io = ImGui::GetIO();
		if (io.KeyShift)
		{
			SelectHierarchyRange(entity, io.KeyCtrl);
		}
		else if (io.KeyCtrl)
		{
			s_EditorSelection.Toggle(entity);
		}
		else
		{
			s_EditorSelection.SelectOnly(entity);
		}
	}
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		StartRenaming(entity);
	}
	if (ImGui::IsItemHovered())
	{
		if (const ModelComponent* model = s_Scene.TryGet<ModelComponent>(entity))
		{
			ImGui::SetTooltip("%s\n%s", GetEntityKindName(kind), model->FilePath.c_str());
		}
		else
		{
			ImGui::SetTooltip("%s", GetEntityKindName(kind));
		}
	}

	// Drag the entity onto another to make it a child
	if (CanBeReparented(kind) && ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload(s_EntityPayload, &entity, sizeof(entity));
		ImGui::Text("%s", name.c_str());
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget())
	{
		if (CanHaveChildren(kind))
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_EntityPayload))
			{
				requests.Reparent = *(const EnvMapVulkanEntityID*)payload->Data;
				requests.NewParent = entity;
			}
		}
		AcceptMaterialDropOnEntity(entity);
		ImGui::EndDragDropTarget();
	}

	// Context menu
	if (ImGui::BeginPopupContextItem("##context"))
	{
		// On a selected row the menu is for the whole selection (Delete); on another row, for that entity alone
		if (!s_EditorSelection.IsSelected(entity))
		{
			s_EditorSelection.SelectOnly(entity);
		}
		else if (s_EditorSelection.GetPrimary() != entity)
		{
			s_EditorSelection.SetPrimary(entity);
		}
		if (ImGui::MenuItem("Rename", "F2"))
		{
			StartRenaming(entity);
		}
		if (ImGui::MenuItem("Move to Top Level", nullptr, false, CanBeReparented(kind) && s_Scene.GetParent(entity) != NoEntity))
		{
			requests.Reparent = entity;
			requests.NewParent = NoEntity;
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Copy", "Ctrl+C", false, CanBeCopied(kind) || s_EditorSelection.GetAll().size() > 1))
		{
			CopySelection();
		}
		if (ImGui::MenuItem("Paste", "Ctrl+V", false, CanPaste()))
		{
			RequestPaste();
		}
		ImGui::Separator();
		const size_t selectedCount = s_EditorSelection.GetAll().size();
		const std::string deleteLabel = selectedCount > 1 ? "Delete " + std::to_string(selectedCount) + " Selected" : std::string("Delete");
		if (ImGui::MenuItem(deleteLabel.c_str(), "Delete", false, selectedCount > 1 || CanBeDeleted(kind)))
		{
			requests.DeleteSelection = true;
		}
		ImGui::EndPopup();
	}

	// The marker, and the name field while renaming
	const float markerX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing() + 5.0f;
	const float centerY = (itemMin.y + itemMax.y) * 0.5f;
	ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(markerX, centerY), 4.5f, GetEntityKindColor(kind));
	if (s_RenamingEntity == entity)
	{
		ImGui::SameLine(markerX - ImGui::GetWindowPos().x + ImGui::GetScrollX() + 10.0f);
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::IsWindowAppearing() || !ImGui::IsAnyItemActive())
		{
			ImGui::SetKeyboardFocusHere();
		}
		if (ImGui::InputText("##rename", s_RenameBuffer, sizeof(s_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
		{
			if (s_RenameBuffer[0] != '\0')
			{
				s_Scene.Get<NameComponent>(entity).Name = s_RenameBuffer;
			}
			s_RenamingEntity = NoEntity;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_Escape) || (ImGui::IsItemDeactivated() && !ImGui::IsItemDeactivatedAfterEdit()))
		{
			s_RenamingEntity = NoEntity;
		}
		else if (ImGui::IsItemDeactivatedAfterEdit())
		{
			if (s_RenameBuffer[0] != '\0')
			{
				s_Scene.Get<NameComponent>(entity).Name = s_RenameBuffer; // clicking elsewhere applies the name too
			}
			s_RenamingEntity = NoEntity;
		}
	}

	if (open)
	{
		for (EnvMapVulkanEntityID child : std::vector<EnvMapVulkanEntityID>(children)) // a copy: the tree isn't changed while it's drawn, but be safe
		{
			DrawHierarchyNode(child, requests);
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

void OnImGuiRenderSceneHierarchy()
{
	ImGui::SetNextWindowSize(ImVec2(320.0f, 360.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene Hierarchy");

	// Add: a model file, a light, the water, a sound
	if (ImGui::Button("Add..."))
	{
		ImGui::OpenPopup("##AddEntity");
	}
	if (ImGui::BeginPopup("##AddEntity"))
	{
		if (ImGui::MenuItem("Model..."))
		{
			std::string filepath = FileDialogs::OpenFile({}, GetAssetFolderForDialog());
			if (!filepath.empty())
			{
				s_PendingModelFilename = filepath; // loaded at the start of the next frame (see EditorPreFrame)
				s_PendingModelGroundPosition.reset();
			}
		}
		if (ImGui::MenuItem("Point Light", nullptr, false, s_Lights.CanAddPointLight()))
		{
			AddPointLightAtView();
		}
		if (ImGui::MenuItem("Spot Light", nullptr, false, s_Lights.CanAddSpotLight()))
		{
			AddSpotLightAtView();
		}
		if (ImGui::MenuItem("Water", nullptr, false, s_WaterEntity == NoEntity))
		{
			AddWaterAtView();
		}
		if (ImGui::MenuItem("Probe Volume", nullptr, false, s_ProbeVolumeEntity == NoEntity))
		{
			AddProbeVolumeAtView();
		}
		if (ImGui::MenuItem("Sound"))
		{
			AddSoundAtView();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("A sound that belongs to no model: music, a waterfall, a humming machine.\n"
				"Choose its clip in the Inspector. A model gets its own sound in its Audio section.");
		}
		ImGui::EndPopup();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Load a model, add a point or spot light (up to %u each), the water or a probe volume (one each per scene).\n"
			"Models can also be dropped here or onto the viewport from the Content Browser", EnvMapVulkanLightsGPU::MaxPointLights);
	}
	ImGui::SameLine();
	ImGui::Checkbox("Light Icons", &s_ShowLightGizmos);
	ImGui::TextDisabled("Scene: %s%s", s_SceneName.c_str(), s_SceneHasUnsavedChanges ? " *" : "");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s%s", s_SceneFilePath.empty() ? "Not saved yet (File > Save Scene)" : s_SceneFilePath.c_str(),
			s_SceneHasUnsavedChanges ? "\n* unsaved changes" : "");
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Light icons in the viewport (click one to select the light), and the selected light's range and cone");
	}
	ImGui::Separator();

	// The tree; a new selection (from anywhere: the viewport, the Inspector) is revealed in it
	static EnvMapVulkanEntityID s_LastSelected = NoEntity;
	s_RevealSelection = s_EditorSelection.GetPrimary() != s_LastSelected && s_EditorSelection.GetPrimary() != NoEntity;
	s_LastSelected = s_EditorSelection.GetPrimary();
	HierarchyRequests requests;
	ImGui::BeginChild("##HierarchyTree", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
	s_EditorSelection.Sync();
	s_HierarchyRowsDrawing.clear();
	for (EnvMapVulkanEntityID root : std::vector<EnvMapVulkanEntityID>(s_Scene.GetRoots()))
	{
		DrawHierarchyNode(root, requests);
	}
	s_HierarchyRows.swap(s_HierarchyRowsDrawing);
	// The empty area below the tree: drop an entity there to move it to the top level, a model file to load it
	ImVec2 freeSize = ImGui::GetContentRegionAvail();
	ImGui::InvisibleButton("##HierarchyFreeArea", ImVec2(std::max(freeSize.x, 1.0f), std::max(freeSize.y, 40.0f)));
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift)
	{
		s_EditorSelection.SelectOnly(NoEntity);
	}
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_EntityPayload))
		{
			requests.Reparent = *(const EnvMapVulkanEntityID*)payload->Data;
			requests.NewParent = NoEntity;
		}
		std::string filepath;
		if (AcceptFileDrop(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), EnvMapVulkanSceneLoader::IsModelFile, "a model file", filepath))
		{
			s_PendingModelFilename = filepath; // loaded at the start of the next frame (see EditorPreFrame)
			s_PendingModelGroundPosition.reset();
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::EndChild();

	// Keys, while the panel has the focus: F2 renames the selected (primary) entity, Delete removes the selected ones,
	// Ctrl+C and Ctrl+V copy and paste them
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		HandleCopyPasteKeys();
	}
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && s_RenamingEntity == NoEntity && s_Scene.Exists(s_EditorSelection.GetPrimary()))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_F2))
		{
			StartRenaming(s_EditorSelection.GetPrimary());
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			requests.DeleteSelection = true;
		}
	}

	// The requests, now that the tree is drawn
	if (requests.Reparent != NoEntity)
	{
		if (!s_Scene.SetParent(requests.Reparent, requests.NewParent, true))
		{
			Log::GetLogger()->warn("'{0}' can't be moved under one of its own children", s_Scene.Get<NameComponent>(requests.Reparent).Name);
		}
		ExtractLights();
	}
	if (requests.DeleteSelection)
	{
		DeleteSelectedEntities();
	}

	ImGui::End();
}
