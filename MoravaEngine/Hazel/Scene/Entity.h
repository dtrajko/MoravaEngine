#pragma once

#include "../../Scene.h"
#include "../../Log.h"
#include "../../Mesh.h"
#include "../../Material.h"

#include "entt.hpp"


namespace Hazel {

	class Entity {

	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene);
		Entity(const std::string& name);
		Entity(const Entity& other) = default;

		void SetMaterial(Material* material) { m_Material = material; }
		Material* GetMaterial() { return m_Material; }

		const glm::mat4& GetTransform() const { return m_Transform; }
		glm::mat4& Transform() { return m_Transform; }

		const std::string& GetName() const { return m_Name; }

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			if (HasComponent<T>())
			{
				Log::GetLogger()->error("Entity already has component!");
			}

			return m_Scene->GetRegistry()->emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			if (!HasComponent<T>())
			{
				Log::GetLogger()->error("Entity does not have component!");
			}

			return m_Scene->GetRegistry()->get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->GetRegistry()->has<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			if (!HasComponent<T>())
			{
				Log::GetLogger()->error("Entity does not have component!");
			}

			m_Scene->GetRegistry()->remove<T>(m_EntityHandle);
		}

		// TODO: Move to Component
		void SetMesh(Mesh* mesh) { m_Mesh = mesh; }
		Mesh* GetMesh() { return m_Mesh; }

		operator bool() const { return m_EntityHandle != entt::null; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }
		bool operator==(const Entity& other) const {
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		};
		bool operator!=(const Entity& other) const {
			return !(*this == other);
		};

	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene = nullptr;

		std::string m_Name;
		glm::mat4 m_Transform;

		Mesh* m_Mesh;
		Material* m_Material;

	};

}
