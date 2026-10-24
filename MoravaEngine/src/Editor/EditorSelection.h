#pragma once

#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include <vector>


/**
 * The editor's selection: every selected entity, and the primary one (one of them), which the Inspector, the
 * Material Editor and the gizmo work with (the gizmo moves the others along).
 *
 * Setting only the primary entity (a model loaded, a light added, a deselection) selects that entity alone: the list
 * follows at the next Sync. Ctrl + click adds or removes an entity (Toggle), Shift + click in the Scene Hierarchy selects
 * a range of rows from the anchor, the last plain or Ctrl click (SelectRange).
 */
class EditorSelection
{
public:
	explicit EditorSelection(const EnvMapVulkanScene& scene) : m_Scene(scene) {}

	EnvMapVulkanEntityID GetPrimary() const { return m_Primary; }
	void SetPrimary(EnvMapVulkanEntityID entity) { m_Primary = entity; }

	// Removed entities leave the selection; a primary entity that isn't in the list (set on its own) is the selection
	void Sync();
	bool IsSelected(EnvMapVulkanEntityID entity) const;
	// Every selected entity (after Sync)
	const std::vector<EnvMapVulkanEntityID>& GetAll();

	// A plain click: the entity alone (NoEntity: nothing)
	void SelectOnly(EnvMapVulkanEntityID entity);
	// Ctrl + click: adds the entity (it becomes the primary one) or removes it (the last selected one becomes primary)
	void Toggle(EnvMapVulkanEntityID entity);
	// Ctrl or Shift + double click in the viewport: every part of the model joins the selection, with the clicked one as
	// the primary entity. The model itself leaves the selection, so the gizmo doesn't move its parts twice. A model
	// without parts is selected itself.
	void AddModelParts(EnvMapVulkanEntityID model, EnvMapVulkanEntityID clicked);
	// Shift + click on a row: the rows from the anchor to the clicked one (added to the selection with add), the clicked
	// one primary. rows: every row in the order shown.
	void SelectRange(const std::vector<EnvMapVulkanEntityID>& rows, EnvMapVulkanEntityID entity, bool add);
	// These entities alone (e.g. pasted ones), the last one primary
	void Set(const std::vector<EnvMapVulkanEntityID>& entities);

	// The meshes of a model to show as selected: { -1 } when the model itself is selected, else the mesh indices of its
	// selected parts (empty: nothing of it is selected)
	std::vector<int> GetSelectedMeshes(EnvMapVulkanEntityID model) const;

private:
	const EnvMapVulkanScene& m_Scene;
	std::vector<EnvMapVulkanEntityID> m_Selection;
	EnvMapVulkanEntityID m_Primary = NoEntity;
	EnvMapVulkanEntityID m_Anchor = NoEntity;
};
