#pragma once

#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class CameraController
{

public:
	CameraController();
	CameraController(Camera* camera, float aspectRatio, float moveSpeed, float turnSpeed);
	virtual ~CameraController();

	virtual void Update();
	void OnEvent(Event& e);

	virtual void KeyControl(bool* keys, float deltaTime);
	virtual void MouseControl(bool* buttons, float xChange, float yChange);
	virtual void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset);
	inline Camera* GetCamera() { return m_Camera; };
	void InvertPitch();
	glm::mat4 CalculateViewMatrix();
	inline void SetUnlockRotation(bool unlockRotation) { m_UnlockRotation = unlockRotation; };

	void OnResize(uint32_t width, uint32_t height);

	inline void SetZoomLevel(float zoomLevel) { m_ZoomLevel = zoomLevel; }
	inline float GetZoomLevel() const { return m_ZoomLevel; }
	inline float GetAspectRatio() const { return m_AspectRatio; }

private:
	void CalculateFront();

private:
	Camera* m_Camera;

protected:
	float m_AspectRatio;
	float m_ZoomLevel = 1.0f;


	float m_MoveSpeed;
	float m_TurnSpeed;
	float m_SpeedBoost = 4.0f;

	float m_MouseDeltaX = 0.0f;
	float m_MouseDeltaY = 0.0f;

	bool m_UnlockRotation; // Left SHIFT for mouse rotation

	// Hazel Camera::MouseRotate
	glm::vec2 m_InitialMousePosition;
	float m_Yaw = 0.0f;
	float m_Pitch = 0.0f;
	float m_YawSign = 0.0f;
	float m_RotationSpeed = 0.01f;

};
