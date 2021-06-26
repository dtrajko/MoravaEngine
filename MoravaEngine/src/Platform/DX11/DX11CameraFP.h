#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11InputListener.h"

#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Renderer/HazelCamera.h"


class DX11CameraFP : public Hazel::HazelCamera, public DX11InputListener
{
public:
	DX11CameraFP();
	DX11CameraFP(glm::mat4 projection);
	~DX11CameraFP();

	void OnUpdate(Hazel::Timestep ts);

	// static DX11CameraFP* Get();

	// Inherited via DX11InputListener
	virtual void OnKeyDown(int key) override;
	virtual void OnKeyUp(int key) override;

	// MOUSE pure virtual callback functions
	virtual void OnMouseMove(const glm::vec2& mousePosDelta, const glm::vec2& mousePosAbs) override;

	virtual void OnLeftMouseDown(const glm::vec2& mousePos) override;
	virtual void OnRightMouseDown(const glm::vec2& mousePos) override;

	virtual void OnLeftMouseUp(const glm::vec2& mousePos) override;
	virtual void OnRightMouseUp(const glm::vec2& mousePos) override;

	virtual void SetViewportSize(float width, float height) override;

	// Methods from EditorCamera (Hazel)
	virtual void OnEvent(Event& e) override;

	void SetEnabled(bool enabled) { m_Enabled = enabled; }
	bool IsEnabled() { return m_Enabled; }

private:
	void UpdateProjection();
	void UpdateView();

private:
	float m_MoveSpeed = 2.0f;
	float m_TurnSpeed = 5.0f;
	float m_SpeedBoost = 4.0f;
	bool m_SpeedBoostEnabled = false;

	bool m_Enabled;

};
