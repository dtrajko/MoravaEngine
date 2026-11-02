#include "EditorSelection.h"

// The mesh parts' material (MeshPartComponent::Material): its type is needed wherever components are kept in a registry
#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"

#include <algorithm>


template<typename T>
static bool Contains(const std::vector<T>& values, const T& value)
{
	return std::find(values.begin(), values.end(), value) != values.end();
}

void EditorSelection::Sync()
{
	m_Selection.erase(std::remove_if(m_Selection.begin(), m_Selection.end(),
		[this](EnvMapVulkanEntityID entity) { return !m_Scene.Exists(entity); }), m_Selection.end());
	if (m_Primary != NoEntity && !m_Scene.Exists(m_Primary))
	{
		m_Primary = m_Selection.empty() ? NoEntity : m_Selection.back();
	}
	if (m_Primary == NoEntity)
	{
		m_Selection.clear();
	}
	else if (!Contains(m_Selection, m_Primary))
	{
		m_Selection = { m_Primary };
	}
}

bool EditorSelection::IsSelected(EnvMapVulkanEntityID entity) const
{
	if (entity == NoEntity)
	{
		return false;
	}
	// Before Sync, a primary entity set on its own is the whole selection
	return entity == m_Primary || (Contains(m_Selection, m_Primary) && Contains(m_Selection, entity));
}

const std::vector<EnvMapVulkanEntityID>& EditorSelection::GetAll()
{
	Sync();
	return m_Selection;
}

void EditorSelection::SelectOnly(EnvMapVulkanEntityID entity)
{
	m_Primary = entity;
	m_Selection.clear();
	if (entity != NoEntity)
	{
		m_Selection.push_back(entity);
	}
	m_Anchor = entity;
}

void EditorSelection::Toggle(EnvMapVulkanEntityID entity)
{
	if (entity == NoEntity)
	{
		return;
	}
	Sync();
	auto it = std::find(m_Selection.begin(), m_Selection.end(), entity);
	if (it != m_Selection.end())
	{
		m_Selection.erase(it);
		m_Primary = m_Selection.empty() ? NoEntity : m_Selection.back();
	}
	else
	{
		m_Selection.push_back(entity);
		m_Primary = entity;
	}
	m_Anchor = entity;
}

void EditorSelection::AddModelParts(EnvMapVulkanEntityID model, EnvMapVulkanEntityID clicked)
{
	Sync();
	std::vector<EnvMapVulkanEntityID> parts;
	for (EnvMapVulkanEntityID child : m_Scene.GetChildren(model))
	{
		if (m_Scene.Has<MeshPartComponent>(child))
		{
			parts.push_back(child);
		}
	}
	if (parts.empty())
	{
		parts.push_back(model);
	}
	else
	{
		m_Selection.erase(std::remove(m_Selection.begin(), m_Selection.end(), model), m_Selection.end());
	}
	EnvMapVulkanEntityID primary = Contains(parts, clicked) ? clicked : parts.back();
	for (EnvMapVulkanEntityID part : parts)
	{
		if (part != primary && !Contains(m_Selection, part))
		{
			m_Selection.push_back(part);
		}
	}
	m_Selection.erase(std::remove(m_Selection.begin(), m_Selection.end(), primary), m_Selection.end());
	m_Selection.push_back(primary); // the primary entity is the last one, as after a Ctrl + click
	m_Primary = primary;
	m_Anchor = primary;
}

void EditorSelection::SelectRange(const std::vector<EnvMapVulkanEntityID>& rows, EnvMapVulkanEntityID entity, bool add)
{
	auto anchor = std::find(rows.begin(), rows.end(), m_Anchor);
	auto clicked = std::find(rows.begin(), rows.end(), entity);
	if (anchor == rows.end() || clicked == rows.end())
	{
		SelectOnly(entity); // no anchor (yet): the row alone
		return;
	}
	if (!add)
	{
		m_Selection.clear();
	}
	else
	{
		Sync();
	}
	for (auto it = std::min(anchor, clicked); it <= std::max(anchor, clicked); ++it)
	{
		if (!Contains(m_Selection, *it))
		{
			m_Selection.push_back(*it);
		}
	}
	m_Primary = entity; // the anchor stays where it was
}

void EditorSelection::Set(const std::vector<EnvMapVulkanEntityID>& entities)
{
	m_Selection = entities;
	m_Primary = entities.empty() ? NoEntity : entities.back();
	m_Anchor = m_Primary;
}

std::vector<int> EditorSelection::GetSelectedMeshes(EnvMapVulkanEntityID model) const
{
	if (IsSelected(model))
	{
		return { -1 };
	}
	std::vector<int> meshes;
	for (EnvMapVulkanEntityID child : m_Scene.GetChildren(model))
	{
		const MeshPartComponent* part = m_Scene.TryGet<MeshPartComponent>(child);
		if (part && IsSelected(child))
		{
			meshes.push_back((int)part->MeshIndex);
		}
	}
	return meshes;
}
