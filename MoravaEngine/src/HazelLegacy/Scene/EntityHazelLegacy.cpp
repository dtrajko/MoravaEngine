#include "EntityHazelLegacy.h"

#include "HazelLegacy/Scene/ComponentsHazelLegacy.h"
#include "HazelLegacy/Scene/SceneHazelLegacy.h"


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

	template<typename T>
	T& EntityHazelLegacy::GetComponent()
	{
		HZ_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
		return m_Scene->m_Registry.get<T>(m_EntityHandle);
	}

	TransformComponentHazelLegacy& EntityHazelLegacy::Transform()
	{
		return m_Scene->m_Registry.get<TransformComponentHazelLegacy>(m_EntityHandle);
	}

	glm::mat4 EntityHazelLegacy::Transform() const
	{
		return m_Scene->m_Registry.get<TransformComponentHazelLegacy>(m_EntityHandle).GetTransform();
	}

	bool EntityHazelLegacy::HasParent()
	{
		return m_Scene->FindEntityByUUID(GetParentUUID());
	}

	bool EntityHazelLegacy::IsAncesterOf(EntityHazelLegacy entity)
	{
		const auto& children = Children();

		if (children.size() == 0)
			return false;

		for (UUID child : children)
		{
			if (child == entity.GetUUID())
				return true;
		}

		for (UUID child : children)
		{
			if (m_Scene->FindEntityByUUID(child).IsAncesterOf(entity))
				return true;
		}

		return false;
	}

	UUID EntityHazelLegacy::GetSceneUUID()
	{
		return m_Scene->GetUUID();
	}

}
