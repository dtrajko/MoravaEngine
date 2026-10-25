#pragma once

#include "EnvMapVulkanComponents.h"

#include "entt.hpp"

#include <functional>
#include <unordered_map>
#include <vector>


/**
 * The scene of SceneEnvMapVulkan: entities in a tree, each with an ID, a name, a place in the hierarchy and a transform,
 * plus the components that say what it is (EnvMapVulkanComponents.h). Built on EnTT; the renderer reads it every frame
 * and keeps no scene state of its own.
 *
 * Entities are addressed by ID everywhere outside this class (EnvMapVulkanEntityID); the registry handles stay inside.
 * The root entities (no parent) are kept in order, as are each entity's children, so the hierarchy panel shows a stable
 * order.
 */
class EnvMapVulkanScene
{
public:
	EnvMapVulkanScene() = default;
	// Not by accident: a copy is made with CopyFrom
	EnvMapVulkanScene(const EnvMapVulkanScene&) = delete;
	EnvMapVulkanScene& operator=(const EnvMapVulkanScene&) = delete;

	// A new entity with an ID, a name, a hierarchy place and an identity transform, appended to the parent's children
	// (to the roots without a parent). id: NoEntity for a new random ID, or a given one (a loaded scene).
	EnvMapVulkanEntityID CreateEntity(const std::string& name, EnvMapVulkanEntityID parent = NoEntity, EnvMapVulkanEntityID id = NoEntity);
	// Removes the entity and all its descendants
	void DestroyEntity(EnvMapVulkanEntityID id);
	void Clear();

	// This scene becomes a copy of another: the same entities with the same IDs (a selection made in one is valid in the
	// other), in the same hierarchy, with a copy of every component of SceneComponents. The models and materials the
	// components refer to are shared, not loaded again.
	void CopyFrom(const EnvMapVulkanScene& other);
	// A copy of an entity and its descendants (of another scene, or of this one) under parent, as its last child: every
	// component of SceneComponents copied (models and materials shared), the hierarchy's order kept. Each copy's ID is
	// newId(i), i counting the copies in the hierarchy's order from 0 (the root), or a new random one when newId returns
	// NoEntity or an ID in use. Script fields that point inside the subtree point at the copies. Returns the root's copy;
	// copies (when given) gets each source entity's copy.
	EnvMapVulkanEntityID CopySubtree(const EnvMapVulkanScene& from, EnvMapVulkanEntityID root, EnvMapVulkanEntityID parent,
		const std::function<EnvMapVulkanEntityID(size_t)>& newId = nullptr,
		std::unordered_map<EnvMapVulkanEntityID, EnvMapVulkanEntityID>* copies = nullptr);
	// The entity and its descendants in the hierarchy's order (the root first, each entity before its children)
	std::vector<EnvMapVulkanEntityID> GetSubtree(EnvMapVulkanEntityID root) const;

	bool Exists(EnvMapVulkanEntityID id) const { return m_Entities.find(id) != m_Entities.end(); }
	size_t GetEntityCount() const { return m_Entities.size(); }
	const std::vector<EnvMapVulkanEntityID>& GetRoots() const { return m_Roots; }
	const std::vector<EnvMapVulkanEntityID>& GetChildren(EnvMapVulkanEntityID id) const;
	EnvMapVulkanEntityID GetParent(EnvMapVulkanEntityID id) const;
	bool IsDescendantOf(EnvMapVulkanEntityID id, EnvMapVulkanEntityID ancestor) const;

	// Moves the entity under a new parent (NoEntity: to the roots), at the end of its children or at position index.
	// keepWorldTransform: the entity stays where it is in the world (its local transform changes). Refused (false) when
	// the new parent is the entity itself or one of its descendants.
	bool SetParent(EnvMapVulkanEntityID id, EnvMapVulkanEntityID parent, bool keepWorldTransform = true, int index = -1);

	// The entity's transform in the world: its parents' transforms and its own
	glm::mat4 GetWorldTransform(EnvMapVulkanEntityID id) const;
	// Sets the local transform so that the entity ends up at this world transform
	void SetWorldTransform(EnvMapVulkanEntityID id, const glm::mat4& world);

	// Components, by entity ID
	template<typename T, typename... Args>
	T& Add(EnvMapVulkanEntityID id, Args&&... args) { return m_Registry.emplace<T>(Handle(id), std::forward<Args>(args)...); }
	template<typename T>
	T& Get(EnvMapVulkanEntityID id) { return m_Registry.get<T>(Handle(id)); }
	template<typename T>
	const T& Get(EnvMapVulkanEntityID id) const { return m_Registry.get<T>(Handle(id)); }
	template<typename T>
	T* TryGet(EnvMapVulkanEntityID id) { return Exists(id) ? m_Registry.try_get<T>(Handle(id)) : nullptr; }
	template<typename T>
	const T* TryGet(EnvMapVulkanEntityID id) const { return Exists(id) ? m_Registry.try_get<T>(Handle(id)) : nullptr; }
	template<typename T>
	bool Has(EnvMapVulkanEntityID id) const { return Exists(id) && m_Registry.all_of<T>(Handle(id)); }
	template<typename T>
	void Remove(EnvMapVulkanEntityID id) { m_Registry.remove<T>(Handle(id)); }

	// Calls f(id, components&...) for every entity that has all the components
	template<typename... T, typename F>
	void Each(F&& f)
	{
		auto view = m_Registry.view<IDComponent, T...>();
		for (entt::entity entity : view)
		{
			f(view.template get<IDComponent>(entity).ID, view.template get<T>(entity)...);
		}
	}
	template<typename... T, typename F>
	void Each(F&& f) const
	{
		auto view = m_Registry.view<const IDComponent, const T...>();
		for (entt::entity entity : view)
		{
			f(view.template get<const IDComponent>(entity).ID, view.template get<const T>(entity)...);
		}
	}
	// The first entity with the component (NoEntity: none), for the scene's single entities (the environment, the sun,
	// the water)
	template<typename T>
	EnvMapVulkanEntityID FindFirst() const
	{
		auto view = m_Registry.view<const IDComponent, const T>();
		for (entt::entity entity : view)
		{
			return view.template get<const IDComponent>(entity).ID;
		}
		return NoEntity;
	}

	static EnvMapVulkanEntityID NewID();

	// Checks the scene's operations on a scratch scene (create, parent, world transforms, reparent, destroy); logs and
	// returns false on the first failure
	static bool SelfTest();

private:
	entt::entity Handle(EnvMapVulkanEntityID id) const;
	std::vector<EnvMapVulkanEntityID>& SiblingList(EnvMapVulkanEntityID parent); // the parent's children, or the roots

	entt::registry m_Registry;
	std::unordered_map<EnvMapVulkanEntityID, entt::entity> m_Entities;
	std::vector<EnvMapVulkanEntityID> m_Roots;
};
