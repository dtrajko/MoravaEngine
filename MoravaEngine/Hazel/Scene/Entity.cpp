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

}
