#pragma once

#include "entt.hpp"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Scene/ComponentsH2M.h"

#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Scene/SceneH2M.h"

#include "Core/Log.h"
#include "Material/Material.h"


namespace H2M
{

	struct TransformComponentH2M;

	class EntityH2M
	{
	public:
		EntityH2M();
		EntityH2M(entt::entity handle, SceneH2M* scene);
		EntityH2M(const EntityH2M& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			H2M_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			H2M_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
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
			H2M_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		TransformComponentH2M& Transform();
		glm::mat4 Transform() const;

		void SetMaterial(Material* material) { m_Material = material; }
		Material* GetMaterial() { return m_Material; }

		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }
		operator bool() const { return (uint32_t)m_EntityHandle && m_Scene; }

		inline bool operator==(const EntityH2M& other) const {
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		};
		inline bool operator!=(const EntityH2M& other) const {
			return !(*this == other);
		};

		void SetParentUUID(UUID_H2M parent) { GetComponent<RelationshipComponentH2M>().ParentHandle = parent; }
		UUID_H2M GetParentUUID() { return GetComponent<RelationshipComponentH2M>().ParentHandle; }
		std::vector<UUID_H2M>& Children() { return GetComponent<RelationshipComponentH2M>().Children; }

		bool HasParent();

		bool IsAncesterOf(EntityH2M entity);

		bool IsDescendantOf(EntityH2M entity)
		{
			return entity.IsAncesterOf(*this);
		}

		UUID_H2M GetUUID() { return GetComponent<IDComponentH2M>().ID; }

		UUID_H2M GetSceneUUID();

		inline uint32_t GetHandle() { return (uint32_t)m_EntityHandle; }

		inline bool IsValid() { return m_EntityHandle != entt::null && (int)m_EntityHandle != 0 && std::abs((int)m_EntityHandle) < 100000; }

	private:
		EntityH2M(const std::string& name);

	private:
		entt::entity m_EntityHandle{ entt::null };
		SceneH2M* m_Scene;

		Material* m_Material;

		friend class SceneH2M;
		friend class ScriptEngine;

	};

}
