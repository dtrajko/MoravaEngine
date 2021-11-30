#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/ImGui/ImGuiLayer.h"


class DX11ImGuiLayer : public H2M::ImGuiLayer
{
public:
	DX11ImGuiLayer();
	DX11ImGuiLayer(const std::string& name);
	virtual ~DX11ImGuiLayer();

	virtual void Begin() override;
	virtual void End() override;

	virtual void OnAttach();
	virtual void OnDetach();
	virtual void OnUpdate(H2M::Timestep ts) override;
	virtual void OnEvent(Event& event) override;
	virtual void OnRender() override;
	virtual void OnImGuiRender() override;

private:
	float m_Time = 0.0f;

};
