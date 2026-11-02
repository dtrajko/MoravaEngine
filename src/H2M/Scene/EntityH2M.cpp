/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "EntityH2M.h"

#include "H2M/Scene/ComponentsH2M.h"
#include "H2M/Scene/SceneH2M.h"


namespace H2M {

	EntityH2M::EntityH2M()
	{
		m_Material = nullptr;

		m_EntityHandle = entt::null;
		m_Scene = nullptr;
	}

	EntityH2M::EntityH2M(entt::entity handle, SceneH2M* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	TransformComponentH2M& EntityH2M::Transform()
	{
		return m_Scene->m_Registry.get<TransformComponentH2M>(m_EntityHandle);
	}

	glm::mat4 EntityH2M::Transform() const
	{
		return m_Scene->m_Registry.get<TransformComponentH2M>(m_EntityHandle).GetTransform();
	}

	bool EntityH2M::HasParent()
	{
		return m_Scene->FindEntityByUUID(GetParentUUID());
	}

	bool EntityH2M::IsAncesterOf(EntityH2M entity)
	{
		const auto& children = Children();

		if (children.size() == 0)
			return false;

		for (UUID_H2M child : children)
		{
			if (child == entity.GetUUID())
				return true;
		}

		for (UUID_H2M child : children)
		{
			if (m_Scene->FindEntityByUUID(child).IsAncesterOf(entity))
				return true;
		}

		return false;
	}

	UUID_H2M EntityH2M::GetSceneUUID()
	{
		return m_Scene->GetUUID();
	}

}
