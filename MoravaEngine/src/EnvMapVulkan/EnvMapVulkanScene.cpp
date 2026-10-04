#include "EnvMapVulkanScene.h"

#include "Core/Log.h"

#include "ImGuizmo.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <random>


// TransformComponent

glm::mat4 TransformComponent::GetMatrix() const
{
	glm::mat4 matrix;
	ImGuizmo::RecomposeMatrixFromComponents(&Translation.x, &Rotation.x, &Scale.x, glm::value_ptr(matrix));
	return matrix;
}

void TransformComponent::SetMatrix(const glm::mat4& matrix)
{
	glm::mat4 copy = matrix;
	ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(copy), &Translation.x, &Rotation.x, &Scale.x);
}

glm::vec3 TransformComponent::GetLightDirection() const
{
	glm::vec3 down = glm::vec3(GetMatrix() * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
	float length = glm::length(down);
	return length > 0.0f ? down / length : glm::vec3(0.0f, -1.0f, 0.0f);
}

void TransformComponent::SetLightDirection(const glm::vec3& direction)
{
	if (glm::length(direction) <= 0.0f)
	{
		return;
	}
	// The shortest turn of -Y onto the direction (no roll around it), kept with the translation and the scale
	glm::quat turn = glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), glm::normalize(direction));
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), Translation) * glm::toMat4(turn) * glm::scale(glm::mat4(1.0f), Scale);
	SetMatrix(matrix);
}

// EnvMapVulkanScene

EnvMapVulkanEntityID EnvMapVulkanScene::NewID()
{
	static std::mt19937_64 s_Generator(std::random_device{}());
	EnvMapVulkanEntityID id = NoEntity;
	while (id == NoEntity)
	{
		id = s_Generator();
	}
	return id;
}

entt::entity EnvMapVulkanScene::Handle(EnvMapVulkanEntityID id) const
{
	auto it = m_Entities.find(id);
	return it != m_Entities.end() ? it->second : entt::null;
}

std::vector<EnvMapVulkanEntityID>& EnvMapVulkanScene::SiblingList(EnvMapVulkanEntityID parent)
{
	return parent == NoEntity ? m_Roots : Get<HierarchyComponent>(parent).Children;
}

EnvMapVulkanEntityID EnvMapVulkanScene::CreateEntity(const std::string& name, EnvMapVulkanEntityID parent, EnvMapVulkanEntityID id)
{
	if (id == NoEntity || Exists(id))
	{
		id = NewID();
	}
	if (parent != NoEntity && !Exists(parent))
	{
		parent = NoEntity;
	}
	entt::entity entity = m_Registry.create();
	m_Entities[id] = entity;
	m_Registry.emplace<IDComponent>(entity, id);
	m_Registry.emplace<NameComponent>(entity, name);
	m_Registry.emplace<HierarchyComponent>(entity).Parent = parent;
	m_Registry.emplace<TransformComponent>(entity);
	SiblingList(parent).push_back(id);
	return id;
}

void EnvMapVulkanScene::DestroyEntity(EnvMapVulkanEntityID id)
{
	if (!Exists(id))
	{
		return;
	}
	// The children first (a copy: destroying a child edits this entity's list)
	std::vector<EnvMapVulkanEntityID> children = Get<HierarchyComponent>(id).Children;
	for (EnvMapVulkanEntityID child : children)
	{
		DestroyEntity(child);
	}
	std::vector<EnvMapVulkanEntityID>& siblings = SiblingList(Get<HierarchyComponent>(id).Parent);
	siblings.erase(std::remove(siblings.begin(), siblings.end(), id), siblings.end());
	m_Registry.destroy(Handle(id));
	m_Entities.erase(id);
}

void EnvMapVulkanScene::Clear()
{
	m_Registry.clear();
	m_Entities.clear();
	m_Roots.clear();
}

const std::vector<EnvMapVulkanEntityID>& EnvMapVulkanScene::GetChildren(EnvMapVulkanEntityID id) const
{
	static const std::vector<EnvMapVulkanEntityID> s_None;
	return Exists(id) ? Get<HierarchyComponent>(id).Children : s_None;
}

EnvMapVulkanEntityID EnvMapVulkanScene::GetParent(EnvMapVulkanEntityID id) const
{
	return Exists(id) ? Get<HierarchyComponent>(id).Parent : NoEntity;
}

bool EnvMapVulkanScene::IsDescendantOf(EnvMapVulkanEntityID id, EnvMapVulkanEntityID ancestor) const
{
	for (EnvMapVulkanEntityID parent = GetParent(id); parent != NoEntity; parent = GetParent(parent))
	{
		if (parent == ancestor)
		{
			return true;
		}
	}
	return false;
}

bool EnvMapVulkanScene::SetParent(EnvMapVulkanEntityID id, EnvMapVulkanEntityID parent, bool keepWorldTransform, int index)
{
	if (!Exists(id) || (parent != NoEntity && (!Exists(parent) || parent == id || IsDescendantOf(parent, id))))
	{
		return false;
	}
	const glm::mat4 world = GetWorldTransform(id);

	HierarchyComponent& hierarchy = Get<HierarchyComponent>(id);
	std::vector<EnvMapVulkanEntityID>& oldSiblings = SiblingList(hierarchy.Parent);
	oldSiblings.erase(std::remove(oldSiblings.begin(), oldSiblings.end(), id), oldSiblings.end());

	hierarchy.Parent = parent;
	std::vector<EnvMapVulkanEntityID>& newSiblings = SiblingList(parent);
	if (index < 0 || index >= (int)newSiblings.size())
	{
		newSiblings.push_back(id);
	}
	else
	{
		newSiblings.insert(newSiblings.begin() + index, id);
	}

	if (keepWorldTransform)
	{
		SetWorldTransform(id, world);
	}
	return true;
}

glm::mat4 EnvMapVulkanScene::GetWorldTransform(EnvMapVulkanEntityID id) const
{
	glm::mat4 world(1.0f);
	for (EnvMapVulkanEntityID current = id; current != NoEntity && Exists(current); current = GetParent(current))
	{
		world = Get<TransformComponent>(current).GetMatrix() * world;
	}
	return world;
}

void EnvMapVulkanScene::SetWorldTransform(EnvMapVulkanEntityID id, const glm::mat4& world)
{
	if (!Exists(id))
	{
		return;
	}
	EnvMapVulkanEntityID parent = GetParent(id);
	glm::mat4 local = parent != NoEntity ? glm::inverse(GetWorldTransform(parent)) * world : world;
	Get<TransformComponent>(id).SetMatrix(local);
}

// Self-test

static bool Near(const glm::mat4& a, const glm::mat4& b, float epsilon = 1e-3f)
{
	for (int c = 0; c < 4; c++)
	{
		for (int r = 0; r < 4; r++)
		{
			if (std::abs(a[c][r] - b[c][r]) > epsilon)
			{
				return false;
			}
		}
	}
	return true;
}

bool EnvMapVulkanScene::SelfTest()
{
	auto fail = [](const char* what) {
		Log::GetLogger()->error("EnvMapVulkanScene self-test failed: {0}", what);
		return false;
	};

	EnvMapVulkanScene scene;
	EnvMapVulkanEntityID root = scene.CreateEntity("Root");
	EnvMapVulkanEntityID child = scene.CreateEntity("Child", root);
	EnvMapVulkanEntityID grandchild = scene.CreateEntity("Grandchild", child);
	EnvMapVulkanEntityID other = scene.CreateEntity("Other");
	if (scene.GetEntityCount() != 4 || scene.GetRoots().size() != 2 || scene.GetChildren(root).size() != 1 || scene.GetParent(grandchild) != child)
	{
		return fail("create / hierarchy");
	}
	if (scene.Get<NameComponent>(child).Name != "Child" || scene.Get<IDComponent>(child).ID != child)
	{
		return fail("ID and name components");
	}

	// World transforms: parents' transforms apply to their children
	scene.Get<TransformComponent>(root).Translation = glm::vec3(10.0f, 0.0f, 0.0f);
	scene.Get<TransformComponent>(root).Rotation = glm::vec3(0.0f, 90.0f, 0.0f);
	scene.Get<TransformComponent>(child).Translation = glm::vec3(0.0f, 0.0f, 5.0f);
	scene.Get<TransformComponent>(grandchild).Scale = glm::vec3(2.0f);
	glm::mat4 expected = scene.Get<TransformComponent>(root).GetMatrix() * scene.Get<TransformComponent>(child).GetMatrix() *
		scene.Get<TransformComponent>(grandchild).GetMatrix();
	glm::mat4 grandchildWorld = scene.GetWorldTransform(grandchild);
	if (!Near(grandchildWorld, expected))
	{
		return fail("world transform");
	}

	// Reparenting keeps the world transform; cycles are refused
	if (!scene.SetParent(grandchild, other) || scene.GetChildren(child).size() != 0 || scene.GetParent(grandchild) != other ||
		!Near(scene.GetWorldTransform(grandchild), grandchildWorld))
	{
		return fail("reparent (keep world transform)");
	}
	if (scene.SetParent(root, child) || scene.SetParent(root, root))
	{
		return fail("cycles refused");
	}
	if (!scene.SetParent(child, NoEntity, true, 0) || scene.GetRoots().front() != child)
	{
		return fail("reparent to the roots at a position");
	}

	// Light directions round-trip through the transform
	TransformComponent light;
	glm::vec3 direction = glm::normalize(glm::vec3(0.3f, -0.8f, 0.5f));
	light.SetLightDirection(direction);
	if (glm::length(light.GetLightDirection() - direction) > 1e-3f)
	{
		return fail("light direction");
	}

	// Destroying an entity removes its descendants
	scene.DestroyEntity(other);
	if (scene.Exists(other) || scene.Exists(grandchild) || scene.GetEntityCount() != 2)
	{
		return fail("destroy with descendants");
	}
	if (scene.FindFirst<NameComponent>() == NoEntity)
	{
		return fail("find first");
	}
	scene.Clear();
	if (scene.GetEntityCount() != 0 || !scene.GetRoots().empty())
	{
		return fail("clear");
	}

	Log::GetLogger()->info("EnvMapVulkanScene self-test passed");
	return true;
}
