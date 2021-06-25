#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11InputListener.h"

#include "Hazel/Events/Event.h"


class DX11CameraFP : public DX11InputListener
{
public:
	DX11CameraFP();
	DX11CameraFP(glm::mat4 projection);
	~DX11CameraFP();

	void Update();

	static DX11CameraFP* Get();

	glm::mat4& CalculateViewMatrix();

	// Inherited via DX11InputListener
	virtual void OnKeyDown(int key) override;
	virtual void OnKeyUp(int key) override;

	// MOUSE pure virtual callback functions
	virtual void OnMouseMove(const DX11Point& deltaMousePos) override;

	virtual void OnLeftMouseDown(const DX11Point& deltaMousePos) override;
	virtual void OnRightMouseDown(const DX11Point& deltaMousePos) override;

	virtual void OnLeftMouseUp(const DX11Point& deltaMousePos) override;
	virtual void OnRightMouseUp(const DX11Point& deltaMousePos) override;

	// Methods from EditorCamera (Hazel)
	void OnEvent(Event& e);

	void SetViewportSize(float width, float height);
	void SetProjectionMatrix(glm::mat4 projection);

	void SetEnabled(bool enabled) { m_Enabled = enabled; }
	bool IsEnabled() { return m_Enabled; }

public:
	glm::mat4 m_Projection = glm::mat4(1.0f);
	glm::mat4 m_View = glm::mat4(1.0f);

	glm::vec3 m_CameraPosition = glm::vec3(0.0f, 0.0f, 4.0f);

	glm::vec3 m_CameraVectorFront = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 m_CameraVectorRight = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_CameraVectorUp    = glm::vec3(0.0f, 0.0f, 0.0f);

	float m_CameraYaw   = 90.0f;
	float m_CameraPitch = 0.0f;

private:
	float m_CameraSpeed = 2.0f;
	float m_CameraTurnSpeed = 5.0f;

	bool m_Enabled;

};
