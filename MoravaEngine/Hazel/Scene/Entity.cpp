#include "Entity.h"


namespace Hazel {

	Entity::Entity()
	{
		m_Material = nullptr;
		m_Mesh = nullptr;
		m_Transform = glm::mat4(1.0f);
	}

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{

	}

	Entity::Entity(const std::string& name)
		: m_Name(name), m_Transform(1.0f)
	{

	}

}
