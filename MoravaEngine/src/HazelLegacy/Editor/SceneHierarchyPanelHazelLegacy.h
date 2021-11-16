#pragma once

#include "../../../pch.h"

#include "ImGui/ImGuiWrapper.h"

#include "HazelLegacy/Renderer/MeshHazelLegacy.h"


struct aiNode;

namespace Hazel
{

	class SceneHierarchyPanelHazelLegacy
	{
	public:
		SceneHierarchyPanelHazelLegacy() = default;
		SceneHierarchyPanelHazelLegacy(Ref<HazelScene> scene);
		~SceneHierarchyPanelHazelLegacy();

		void SetContext(Ref<SceneHazelLegacy> scene);
		Ref<SceneHazelLegacy> GetContext() { return m_Context; };
		void SetSelected(EntityHazelLegacy entity);
		void SetSelectionChangedCallback(const std::function<void(EntityHazelLegacy)>& func) { m_SelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(EntityHazelLegacy)>& func) { m_EntityDeletedCallback = func; }
		void SetMeshAssetConvertCallback(const std::function<void(EntityHazelLegacy, Ref<HazelMeshAssetLegacy>)>& func) { m_MeshAssetConvertCallback = func; }
		void SetInvalidMetadataCallback(const std::function<void(EntityHazelLegacy, AssetHandle)>& func) { m_InvalidMetadataCallback = func; }

		void OnImGuiRender(bool* p_open = (bool*)0);

	private:
		void DrawEntityNode(EntityHazelLegacy entity);
		void DrawMeshNode(Mesh* mesh, uint32_t& imguiMeshID);
		void MeshNodeHierarchy(Mesh* mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level);
		void DrawComponents(EntityHazelLegacy entity);
		void DrawEntitySubmeshes(EntityHazelLegacy entity);

	private:
		Ref<SceneHazelLegacy> m_Context;
		Entity m_SelectionContext;

		std::function<void(EntityHazelLegacy)> m_SelectionChangedCallback;
		std::function<void(EntityHazelLegacy)> m_EntityDeletedCallback;
		std::function<void(EntityHazelLegacy, Ref<MeshAsset>)> m_MeshAssetConvertCallback;
		std::function<void(EntityHazelLegacy, AssetHandle)> m_InvalidMetadataCallback;

		Ref<HazelTexture2D> m_PencilIcon;
		Ref<HazelTexture2D> m_PlusIcon;
		Ref<HazelTexture2D> m_GearIcon;

	};

}
