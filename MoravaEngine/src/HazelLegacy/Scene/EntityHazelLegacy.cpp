#include "EntityHazelLegacy.h"
#include "SceneHazelLegacy.h"


namespace Hazel {

	EntityHazelLegacy::EntityHazelLegacy()
	{
		m_Material = nullptr;

		m_EntityHandle = entt::null;
		m_Scene = nullptr;
	}

	EntityHazelLegacy::EntityHazelLegacy(entt::entity handle, SceneHazelLegacy* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	TransformComponent& EntityHazelLegacy::Transform()
	{
		return m_Scene->m_Registry.get<TransformComponent>(m_EntityHandle);
	}

	glm::mat4 EntityHazelLegacy::Transform() const
	{
		return m_Scene->m_Registry.get<TransformComponent>(m_EntityHandle).GetTransform();
	}

}
