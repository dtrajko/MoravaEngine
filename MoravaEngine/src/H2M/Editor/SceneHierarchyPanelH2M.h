/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "../../../pch.h"

#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Scene/SceneH2M.h"

// ImGui
#define _CRT_SECURE_NO_WARNINGS
#include "../ImGui/ImGui.h"

#include <imgui.h>
#include <imgui_internal.h>


struct aiNode;

namespace H2M
{

	class SceneHierarchyPanelH2M
	{
	public:
		SceneHierarchyPanelH2M() = default;
		SceneHierarchyPanelH2M(RefH2M<SceneH2M> scene);
		~SceneHierarchyPanelH2M();

		void SetContext(RefH2M<SceneH2M> scene);
		RefH2M<SceneH2M> GetContext() { return m_Context; };
		void SetSelected(EntityH2M entity);
		void SetSelectionChangedCallback(const std::function<void(EntityH2M)>& func) { m_SelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(EntityH2M)>& func) { m_EntityDeletedCallback = func; }

		void OnImGuiRender(bool* p_open = (bool*)0);

	private:
		void DrawEntityNode(EntityH2M entity);
		void DrawMeshNode(Mesh* mesh, uint32_t& imguiMeshID);
		void MeshNodeHierarchy(Mesh* mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level);
		void DrawComponents(EntityH2M entity);
		void DrawEntitySubmeshes(EntityH2M entity);

	private:
		RefH2M<SceneH2M> m_Context;
		EntityH2M m_SelectionContext;

		std::function<void(EntityH2M)> m_SelectionChangedCallback;
		std::function<void(EntityH2M)> m_EntityDeletedCallback;

	};

}
