#pragma once

#include "../../../pch.h"

// ImGui
#define _CRT_SECURE_NO_WARNINGS
#include "../ImGui/ImGui.h"

#include <imgui.h>
#include <imgui_internal.h>


struct aiNode;

namespace Hazel
{

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(Ref<HazelScene> scene);
		~SceneHierarchyPanel();

		void SetContext(Ref<HazelScene> scene);
		Ref<HazelScene> GetContext() { return m_Context; };
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
