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

}
