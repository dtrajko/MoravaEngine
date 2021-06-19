#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/ImGui/ImGuiLayer.h"


class DX11ImGuiLayer : public Hazel::ImGuiLayer
{
public:
	DX11ImGuiLayer();
	DX11ImGuiLayer(const std::string& name);
	virtual ~DX11ImGuiLayer();

	virtual void Begin() override;
	virtual void End() override;

	virtual void OnAttach();
	virtual void OnDetach();
	virtual void OnImGuiRender();

private:
	float m_Time = 0.0f;

};
