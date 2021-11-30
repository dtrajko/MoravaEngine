#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/Layer.h"
#include "H2M/Core/Timestep.h"
#include "H2M/Editor/ContentBrowserPanel.h"
#include "H2M/Core/Events/Event.h"
#include "H2M/Platform/Vulkan/VulkanPipeline.h"
#include "H2M/Platform/Vulkan/VulkanShader.h"
#include "H2M/Platform/Vulkan/VulkanTexture.h"
#include "H2M/Renderer/SceneRenderer.h"

#include "H2M/Scene/SceneH2M.h"
#include "H2M/Editor/SceneHierarchyPanelH2M.h"

#include "Core/MoravaLayer.h"
#include "Core/Window.h"
#include "Editor/MaterialEditorPanel.h"
#include "Scene/Scene.h"


namespace Hazel {

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
		virtual void OnUpdate(Timestep ts) override;
		virtual void OnEvent(Event& event) override;
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
		static std::vector<Ref<MeshH2M>> s_Meshes;
		Ref<Texture2DH2M> m_Texture;
		EditorCamera m_Camera;

		// std::vector<glm::vec4> m_RandomColors;

	public:
		static Ref<SceneH2M> s_Scene;
		static SceneHierarchyPanelH2M* s_SceneHierarchyPanel;
		static ContentBrowserPanel* s_ContentBrowserPanel;
		static MaterialEditorPanel* s_MaterialEditorPanel;

	};

}
