#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Layer.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Editor/SceneHierarchyPanel.h"
#include "Hazel/Editor/ContentBrowserPanel.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"

#include "Core/MoravaLayer.h"
#include "Core/Window.h"
#include "Editor/MaterialEditorPanel.h"
#include "Scene/Scene.h"


namespace Hazel {

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

		void Render(const glm::vec4& clearColor, const EditorCamera& camera);
		void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);

		void UpdateImGuizmo();

	private:
		std::vector<Ref<HazelMesh>> m_Meshes;
		Ref<HazelTexture2D> m_Texture;

		EditorCamera m_Camera;

		std::vector<glm::vec4> m_RandomColors;

	public:
		static Ref<HazelScene> s_Scene;
		static SceneHierarchyPanel* s_SceneHierarchyPanel;
		static ContentBrowserPanel* s_ContentBrowserPanel;
		static MaterialEditorPanel* s_MaterialEditorPanel;

	};

}
