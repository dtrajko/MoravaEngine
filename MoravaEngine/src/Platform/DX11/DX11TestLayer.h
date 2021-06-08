#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Layer.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"

#include "DX11Pipeline.h"
#include "DX11Shader.h"
#include "DX11Texture.h"

#include "Core/Window.h"
#include "Scene/Scene.h"


class DX11TestLayer : public Hazel::Layer
{
public:
	DX11TestLayer();
	virtual ~DX11TestLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(Hazel::Timestep ts) override;
	void OnImGuiRender(Window* mainWindow, Scene* scene);

	virtual void OnEvent(Event& event) override;

	void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);
	void OnRender(Window* mainWindow);

	void Render(const glm::vec4& clearColor, const Hazel::EditorCamera& camera);

private:
	std::vector<Hazel::Ref<Hazel::HazelMesh>> m_Meshes;
	Hazel::Ref<Hazel::HazelTexture2D> m_Texture;

	Hazel::EditorCamera m_Camera;

	std::vector<glm::vec4> m_RandomColors;

};
