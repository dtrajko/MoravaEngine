#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Layer.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Editor/ContentBrowserPanel.h"
#include "Hazel/Core/Events/Event.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"

#include "HazelLegacy/Scene/SceneHazelLegacy.h"
#include "HazelLegacy/Editor/SceneHierarchyPanelHazelLegacy.h"
#include "HazelLegacy/Renderer/SceneRendererHazelLegacy.h"

#include "Core/MoravaLayer.h"
#include "Core/Window.h"
#include "Editor/MaterialEditorPanel.h"
#include "Scene/Scene.h"


namespace Hazel {

	/**
	 * VulkanTestLayer is to be replaced with SceneRendererHazelLegacy
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
		static std::vector<Ref<MeshHazelLegacy>> s_Meshes;
		Ref<HazelTexture2D> m_Texture;
		EditorCamera m_Camera;

		// std::vector<glm::vec4> m_RandomColors;

	public:
		static Ref<SceneHazelLegacy> s_Scene;
		static SceneHierarchyPanelHazelLegacy* s_SceneHierarchyPanel;
		static ContentBrowserPanel* s_ContentBrowserPanel;
		static MaterialEditorPanel* s_MaterialEditorPanel;

	};

}
