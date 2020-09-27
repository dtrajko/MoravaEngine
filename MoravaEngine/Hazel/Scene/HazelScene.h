#pragma once

#include "Entity.h"

#include "entt.hpp"

#include <string>


namespace Hazel {

	class Entity;

	class HazelScene {

	public:
		HazelScene();
		~HazelScene();

		Entity CreateEntity(const std::string& name = std::string());

		void OnUpdate(float ts);
		void OnViewportResize(uint32_t width, uint32_t height);

	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		friend class Entity;
		friend class SceneHierarchyPanel;

	};

}
