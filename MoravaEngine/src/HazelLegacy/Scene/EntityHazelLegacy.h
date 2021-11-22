#pragma once

#include "entt.hpp"

#include "Hazel/Core/Assert.h"
#include "Hazel/Scene/Components.h"

#include "HazelLegacy/Renderer/MeshHazelLegacy.h"
#include "HazelLegacy/Scene/SceneHazelLegacy.h"

#include "Core/Log.h"
#include "Material/Material.h"


namespace Hazel {

	struct TransformComponentHazelLegacy;

	class EntityHazelLegacy
	{
	public:
		EntityHazelLegacy();
		EntityHazelLegacy(entt::entity handle, SceneHazelLegacy* scene);
		EntityHazelLegacy(const EntityHazelLegacy& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			// HZ_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			HZ_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->m_Registry.has<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			HZ_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		TransformComponentHazelLegacy& Transform();
		glm::mat4 Transform() const;

		void SetMaterial(Material* material) { m_Material = material; }
		Material* GetMaterial() { return m_Material; }

		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }
		operator bool() const { return (uint32_t)m_EntityHandle && m_Scene; }

		inline bool operator==(const EntityHazelLegacy& other) const {
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		};
		inline bool operator!=(const EntityHazelLegacy& other) const {
			return !(*this == other);
		};

		void SetParentUUID(UUID parent) { GetComponent<RelationshipComponent>().ParentHandle = parent; }
		UUID GetParentUUID() { return GetComponent<RelationshipComponent>().ParentHandle; }
		std::vector<UUID>& Children() { return GetComponent<RelationshipComponent>().Children; }

		bool HasParent();

		bool IsAncesterOf(EntityHazelLegacy entity);

		bool IsDescendantOf(EntityHazelLegacy entity)
		{
			return entity.IsAncesterOf(*this);
		}

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }

		UUID GetSceneUUID();

		inline uint32_t GetHandle() { return (uint32_t)m_EntityHandle; }

		inline bool IsValid() { return m_EntityHandle != entt::null && (int)m_EntityHandle != 0 && std::abs((int)m_EntityHandle) < 100000; }

	private:
		EntityHazelLegacy(const std::string& name);

	private:
		entt::entity m_EntityHandle{ entt::null };
		SceneHazelLegacy* m_Scene;

		Material* m_Material;

		friend class SceneHazelLegacy;
		friend class ScriptEngine;

	};

}
