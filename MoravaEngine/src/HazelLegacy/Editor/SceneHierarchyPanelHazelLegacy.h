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

	class SceneHierarchyPanelHazelLegacy
	{
	public:
		SceneHierarchyPanelHazelLegacy() = default;
		SceneHierarchyPanelHazelLegacy(Ref<HazelScene> scene);
		~SceneHierarchyPanelHazelLegacy();

		void SetContext(Ref<HazelScene> scene);
		Ref<HazelScene> GetContext() { return m_Context; };
		void SetSelected(Entity entity);
		void SetSelectionChangedCallback(const std::function<void(Entity)>& func) { m_SelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(Entity)>& func) { m_EntityDeletedCallback = func; }
		void SetMeshAssetConvertCallback(const std::function<void(Entity, Ref<HazelMeshAssetLegacy>)>& func) { m_MeshAssetConvertCallback = func; }
		void SetInvalidMetadataCallback(const std::function<void(Entity, AssetHandle)>& func) { m_InvalidMetadataCallback = func; }

		void OnImGuiRender(bool* p_open = (bool*)0);

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
		std::function<void(Entity, Ref<MeshAsset>)> m_MeshAssetConvertCallback;
		std::function<void(Entity, AssetHandle)> m_InvalidMetadataCallback;

		Ref<HazelTexture2D> m_PencilIcon;
		Ref<HazelTexture2D> m_PlusIcon;
		Ref<HazelTexture2D> m_GearIcon;

	};

}
