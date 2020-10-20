#include "Entity.h"


namespace Hazel {

	Entity::Entity()
	{
		m_Material = nullptr;
		m_Mesh = nullptr;
		m_Transform = glm::mat4(1.0f);

		m_EntityHandle = entt::null;
		m_Scene = nullptr;
	}

	Entity::Entity(entt::entity handle, HazelScene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	Entity::Entity(const std::string& name)
		: m_Name(name), m_Transform(1.0f)
	{
	}

}
