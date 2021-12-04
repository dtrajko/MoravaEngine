#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/LayerH2M.h"
#include "H2M/Core/TimestepH2M.h"
#include "H2M/Editor/ContentBrowserPanelH2M.h"
#include "H2M/Editor/SceneHierarchyPanelH2M.h"
#include "H2M/Core/Events/EventH2M.h"
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"
#include "H2M/Scene/SceneH2M.h"

#include "Core/MoravaLayer.h"
#include "Core/Window.h"
#include "Editor/MaterialEditorPanel.h"
#include "Scene/Scene.h"


namespace H2M {

	/**
	 * VulkanTestLayer is to be replaced with SceneRendererH2M
	 */
	class VulkanTestLayer : public MoravaLayer
	{
	public:
		VulkanTestLayer();
		VulkanTestLayer(const std::string& name);
		virtual ~VulkanTestLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(TimestepH2M ts) override;
		virtual void OnEvent(EventH2M& event) override;
		virtual void OnImGuiRender(::Window* mainWindow, ::Scene* scene) override;
		virtual void OnRender(::Window* mainWindow, ::Scene* scene) override;

		void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);

		// void Render(const EditorCamera& camera);
		// static void MapUniformBuffersVTL(const glm::vec4& clearColor, const EditorCamera& camera);
		// void UpdateImGuizmo(); // we don't need this method anymore
		// static void GeometryPass(const glm::vec4& clearColor, const EditorCamera& camera); // we don't need this method anymore
		// virtual void OnImGuiRenderOld(::Window* mainWindow, ::Scene* scene) override;

		/**** BEGIN moved to VulkanRenderer ****
		static SceneRendererOptions& GetOptions();
		/**** END moved to VulkanRenderer ****/

	private:
		static std::vector<RefH2M<MeshH2M>> s_Meshes;
		RefH2M<Texture2D_H2M> m_Texture;
		EditorCameraH2M m_Camera;

		// std::vector<glm::vec4> m_RandomColors;

	public:
		static RefH2M<SceneH2M> s_Scene;
		static SceneHierarchyPanelH2M* s_SceneHierarchyPanel;
		static ContentBrowserPanelH2M* s_ContentBrowserPanel;
		static MaterialEditorPanel* s_MaterialEditorPanel;

	};

}
