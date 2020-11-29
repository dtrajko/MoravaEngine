#include "Entity.h"


namespace Hazel {

	Entity::Entity()
	{
		m_Material = nullptr;

		m_EntityHandle = entt::null;
		m_Scene = nullptr;
	}

	Entity::Entity(entt::entity handle, HazelScene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	glm::mat4& Entity::Transform()
	{
		glm::mat4 transform = glm::mat4(1.0f);
		if (HasComponent<TransformComponent>()) {
			transform = m_Scene->m_Registry.get<TransformComponent>(m_EntityHandle).GetTransform();
		} else {
			Log::GetLogger()->error("The Entity does not have TransformComponent");
		}
		return transform;
	}

	bool Entity::IsValid()
	{
		if (!m_Scene) return false;

		if (m_Scene->m_Registry.valid(m_EntityHandle)) return false;

		return true;
	}

}
