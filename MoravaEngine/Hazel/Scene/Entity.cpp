#include "Entity.h"
#include "HazelScene.h"


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
		return m_Scene->m_Registry.get<TransformComponent>(m_EntityHandle).GetTransform();
	}

	const glm::mat4& Entity::Transform() const
	{
		return m_Scene->m_Registry.get<TransformComponent>(m_EntityHandle).GetTransform();
	}

	UUID Entity::GetSceneUUID()
	{
		return m_Scene->GetUUID();
	}

}
