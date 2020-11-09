#pragma once

#include "entt.hpp"

#include "HazelScene.h"
#include "../Core/Assert.h"

#include "../../Log.h"
#include "../../Mesh.h"


namespace Hazel {

	class HazelScene;

	class Entity {

	public:
		Entity();
		Entity(entt::entity handle, HazelScene* scene);
		Entity(const std::string& name);
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

		// TODO: Move to Component
		void SetMesh(Mesh* mesh) { m_Mesh = mesh; }
		Mesh* GetMesh() { return m_Mesh; }

		operator bool() const { return m_EntityHandle != entt::null; }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		inline bool operator==(const Entity& other) const {
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		};
		inline bool operator!=(const Entity& other) const {
			return !(*this == other);
		};

		void SetMaterial(Material* material) { m_Material = material; }
		Material* GetMaterial() { return m_Material; }

		// NoECS
		const glm::mat4& GetTransform() const { return m_Transform; }
		glm::mat4& Transform() { return m_Transform; }
		const std::string& GetName() const { return m_Name; }
		inline void SetName(std::string name) { m_Name = name; }

	public:
		Mesh* m_Mesh;

	private:
		entt::entity m_EntityHandle;
		HazelScene* m_Scene = nullptr;

		Material* m_Material;

		// NoECS
		std::string m_Name;
		glm::mat4 m_Transform;

	};

}
