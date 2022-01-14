/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "pch.h"

#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Scene/SceneH2M.h"
#include "H2M/Scene/EntityH2M.h"

// ImGui
#define _CRT_SECURE_NO_WARNINGS
#include "../ImGui/ImGui.h"

#include <imgui.h>
#include <imgui_internal.h>


struct aiNode;

namespace H2M
{

	class SceneHierarchyPanelH2M : public RefCountedH2M // TODO: SceneHierarchyPanelH2M => EditorPanel => RefCountedH2M
	{
	public:
		SceneHierarchyPanelH2M() = default;
		SceneHierarchyPanelH2M(RefH2M<SceneH2M> scene);
		~SceneHierarchyPanelH2M();

		static void Init();
		static void Shutdown();

		void SetContext(RefH2M<SceneH2M> scene);
		RefH2M<SceneH2M> GetContext() { return m_Context; };
		void SetSelected(EntityH2M entity);
		void SetSelectionChangedCallback(const std::function<void(EntityH2M)>& func) { m_SelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(EntityH2M)>& func) { m_EntityDeletedCallback = func; }
		void SetMeshAssetConvertCallback(const std::function<void(EntityH2M, RefH2M<MeshH2M>)>& func) { m_MeshAssetConvertCallback = func; } // TODO: MeshH2M => MeshSourceH2M
		void SetInvalidMetadataCallback(const std::function<void(EntityH2M, AssetHandleH2M)>& func) { m_InvalidMetadataCallback = func; }

		EntityH2M GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(EntityH2M entity) { m_SelectionContext = entity; }

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
		std::function<void(EntityH2M, RefH2M<MeshH2M>)> m_MeshAssetConvertCallback; // TODO: MeshH2M => MeshSourceH2M
		std::function<void(EntityH2M, AssetHandleH2M)> m_InvalidMetadataCallback;

		static RefH2M<Texture2D_H2M> s_PencilIcon;
		static RefH2M<Texture2D_H2M> s_PlusIcon;
		static RefH2M<Texture2D_H2M> s_GearIcon;

	};

}
