#pragma once

#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class CameraController
{

public:
	CameraController();
	CameraController(Camera* camera, float moveSpeed, float turnSpeed);
	~CameraController();

	void KeyControl(bool* keys, float deltaTime);
	void MouseControl(bool* buttons, float xChange, float yChange);
	void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset);
	inline Camera* GetCamera() { return m_Camera; };

private:
	void Update();

private:
	Camera* m_Camera;

	float m_MoveSpeed;
	float m_TurnSpeed;
	float m_SpeedBoost = 4.0f;

	float m_MouseDeltaX = 0.0f;
	float m_MouseDeltaY = 0.0f;

};
