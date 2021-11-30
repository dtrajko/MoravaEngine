#pragma once

#include "Camera/Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class CameraController
{

public:
	CameraController() = default;
	CameraController(H2M::HazelCamera* camera, float aspectRatio, float moveSpeed, float turnSpeed);
	virtual ~CameraController();

	virtual void Update();
	void OnEvent(Event& e);

	virtual void KeyControl(bool* keys, float deltaTime);
	virtual void MouseControl(bool* buttons, float xChange, float yChange);
	virtual void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset);
	inline H2M::HazelCamera* GetCamera() { return m_Camera; };
	void InvertPitch();
	void InvertRoll();
	inline void SetUnlockRotation(bool unlockRotation) { m_UnlockRotation = unlockRotation; };

	void OnResize(uint32_t width, uint32_t height);

	inline void SetZoomLevel(float zoomLevel) { m_ZoomLevel = zoomLevel; }
	inline float GetZoomLevel() const { return m_ZoomLevel; }
	inline float GetAspectRatio() const { return m_Camera->GetAspectRatio(); }

protected:
	void CalculateFront();

protected:
	H2M::HazelCamera* m_Camera;

	float m_ZoomLevel = 1.0f;

	float m_MoveSpeed;
	float m_TurnSpeed;
	float m_SpeedBoost = 4.0f;

	float m_MouseDeltaX = 0.0f;
	float m_MouseDeltaY = 0.0f;

	bool m_UnlockRotation; // Left SHIFT for mouse rotation

	// Hazel Camera::MouseRotate
	glm::vec2 m_InitialMousePosition;
	float m_YawSign = 0.0f;
	float m_RotationSpeed = 0.01f;

};
