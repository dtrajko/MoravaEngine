#pragma once

#include "../Core/Base.h"
#include "../Scene/Entity.h"
#include "../../Scene.h"
#include "../../Log.h"


namespace Hazel
{

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(Scene* context);
		~SceneHierarchyPanel();

		void SetContext(Scene* context);
		void OnImGuiRender();

	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);

	private:
		Scene* m_Context;
		Entity m_SelectionContext;

	};

}
