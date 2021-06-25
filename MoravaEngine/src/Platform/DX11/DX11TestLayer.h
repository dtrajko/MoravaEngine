#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Layer.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"

#include "DX11Pipeline.h"
#include "DX11Shader.h"
#include "DX11Texture2D.h"
#include "DX11VertexBuffer.h"
#include "DX11InputListener.h"
#include "DX11CameraFP.h"

#include "Core/Window.h"
#include "Scene/Scene.h"


class DX11TestLayer : public Hazel::Layer, public DX11InputListener
{
public:
	DX11TestLayer();
	DX11TestLayer(const std::string& name);
	virtual ~DX11TestLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(Hazel::Timestep ts) override;
	void OnImGuiRender(Window* mainWindow, Scene* scene);

	virtual void OnEvent(Event& event) override;

	void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);
	void OnRender(Window* mainWindow);

	void Render(const glm::vec4& clearColor, const DX11CameraFP& camera);

	// Inherited via DX11InputListener
	virtual void OnKeyDown(int key) override;
	virtual void OnKeyUp(int key) override;

	// MOUSE pure virtual callback functions
	virtual void OnMouseMove(const DX11Point& deltaMousePos) override;

	virtual void OnLeftMouseDown(const DX11Point& deltaMousePos) override;
	virtual void OnRightMouseDown(const DX11Point& deltaMousePos) override;

	virtual void OnLeftMouseUp(const DX11Point& deltaMousePos) override;
	virtual void OnRightMouseUp(const DX11Point& deltaMousePos) override;

private:
	std::vector<Hazel::Ref<Hazel::HazelMesh>> m_Meshes;
	Hazel::Ref<Hazel::HazelTexture2D> m_Texture;

	// std::shared_ptr<DX11CameraFP> m_Camera; // TODO: Use DX11CameraFP as m_Camera

	std::vector<glm::vec4> m_RandomColors;

	bool m_ShowMouseCursor;

};
