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

namespace Hazel
{

	class SceneHierarchyPanelH2M
	{
	public:
		SceneHierarchyPanelH2M() = default;
		SceneHierarchyPanelH2M(Ref<HazelScene> scene);
		~SceneHierarchyPanelH2M();

		void SetContext(Ref<SceneH2M> scene);
		Ref<SceneH2M> GetContext() { return m_Context; };
		void SetSelected(EntityH2M entity);
		void SetSelectionChangedCallback(const std::function<void(EntityH2M)>& func) { m_SelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(EntityH2M)>& func) { m_EntityDeletedCallback = func; }
		void SetMeshAssetConvertCallback(const std::function<void(EntityH2M, Ref<HazelMeshAssetLegacy>)>& func) { m_MeshAssetConvertCallback = func; }
		void SetInvalidMetadataCallback(const std::function<void(EntityH2M, AssetHandle)>& func) { m_InvalidMetadataCallback = func; }

		void OnImGuiRender(bool* p_open = (bool*)0);

	private:
		void DrawEntityNode(EntityH2M entity);
		void DrawMeshNode(Mesh* mesh, uint32_t& imguiMeshID);
		void MeshNodeHierarchy(Mesh* mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level);
		void DrawComponents(EntityH2M entity);
		void DrawEntitySubmeshes(EntityH2M entity);

	private:
		Ref<SceneH2M> m_Context;
		Entity m_SelectionContext;

		std::function<void(EntityH2M)> m_SelectionChangedCallback;
		std::function<void(EntityH2M)> m_EntityDeletedCallback;
		std::function<void(EntityH2M, Ref<MeshAsset>)> m_MeshAssetConvertCallback;
		std::function<void(EntityH2M, AssetHandle)> m_InvalidMetadataCallback;

		Ref<Texture2DH2M> m_PencilIcon;
		Ref<Texture2DH2M> m_PlusIcon;
		Ref<Texture2DH2M> m_GearIcon;

	};

}
