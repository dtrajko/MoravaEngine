#pragma once

#include "entt.hpp"

#include "../Core/Assert.h"
#include "Components.h"
#include "HazelScene.h"

#include "../../Log.h"
#include "../../Mesh.h"


namespace Hazel {

	class HazelScene;

	class Entity {

	public:
		Entity();
		Entity(entt::entity handle, Ref<HazelScene>scene);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			HZ_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
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

		glm::mat4& Transform();
		const glm::mat4& Transform() const;

		void SetMaterial(Material* material) { m_Material = material; }
		Material* GetMaterial() { return m_Material; }

		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }
		operator bool() const { return (uint32_t)m_EntityHandle && m_Scene; }

		inline bool operator==(const Entity& other) const {
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		};
		inline bool operator!=(const Entity& other) const {
			return !(*this == other);
		};

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }

		UUID GetSceneUUID();

		inline uint32_t GetHandle() { return (uint32_t)m_EntityHandle; }

	private:
		Entity(const std::string& name);

	private:
		entt::entity m_EntityHandle{ entt::null };
		Ref<HazelScene> m_Scene;

		Material* m_Material;


		friend class HazelScene;
		friend class ScriptEngine;

	};

}
