#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"

#include "Core/Window.h"
#include "Scene/Scene.h"


class VulkanTestLayer
{
public:
	VulkanTestLayer();
	~VulkanTestLayer();

	void OnAttach();
	void OnDetach();

	void OnUpdate(Hazel::Timestep ts, Hazel::HazelCamera* camera); // const Hazel::EditorCamera& camera
	void OnImGuiRender(Window* mainWindow, Scene* scene);

	void OnEvent(Event& event);

	void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);
	void OnRender(Window* mainWindow);

private:
	void Render(const glm::vec4& clearColor, Hazel::HazelCamera* camera); // const Hazel::EditorCamera& camera

private:
	Hazel::Ref<Hazel::HazelMesh> m_Mesh;
	Hazel::Ref<Hazel::HazelTexture2D> m_Texture;

	Hazel::EditorCamera m_Camera;

	std::vector<glm::vec4> m_RandomColors;

};
