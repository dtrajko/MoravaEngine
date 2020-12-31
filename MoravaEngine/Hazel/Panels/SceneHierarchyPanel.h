#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "../Core/Base.h"
#include "../Scene/Entity.h"
#include "../Scene/HazelScene.h"
#include "../Scene/Components.h"

#include "../../Log.h"

#include <functional>


struct aiNode;

namespace Hazel
{

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(HazelScene* scene);
		~SceneHierarchyPanel();

		void SetContext(const Ref<HazelScene>& scene);
		void SetSelected(Entity entity);
		void SetSelectionChangedCallback(const std::function<void(Entity)>& func) { m_SelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(Entity)>& func) { m_EntityDeletedCallback = func; }

		void OnImGuiRender();

	private:
		void DrawEntityNode(Entity entity);
		void DrawMeshNode(Mesh* mesh, uint32_t& imguiMeshID);
		void MeshNodeHierarchy(Mesh* mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level);
		void DrawComponents(Entity entity);
		void DrawEntitySubmeshes(Entity entity);

	private:
		Ref<HazelScene> m_Context;
		Entity m_SelectionContext;

		std::function<void(Entity)> m_SelectionChangedCallback;
		std::function<void(Entity)> m_EntityDeletedCallback;

	};

}
