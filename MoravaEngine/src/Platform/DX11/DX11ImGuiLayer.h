#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/ImGui/ImGuiLayerH2M.h"


class DX11ImGuiLayer : public H2M::ImGuiLayerH2M
{
public:
	DX11ImGuiLayer();
	DX11ImGuiLayer(const std::string& name);
	virtual ~DX11ImGuiLayer();

	virtual void Begin() override;
	virtual void End() override;

	virtual void OnAttach();
	virtual void OnDetach();
	virtual void OnUpdate(H2M::TimestepH2M ts) override;
	virtual void OnEvent(H2M::EventH2M& event) override;
	virtual void OnRender() override;
	virtual void OnImGuiRender() override;

private:
	float m_Time = 0.0f;

};
