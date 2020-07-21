#pragma once

#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class CameraController
{

public:
	CameraController();
	CameraController(Camera* camera, float moveSpeed, float turnSpeed);
	virtual ~CameraController();

	virtual void KeyControl(bool* keys, float deltaTime);
	virtual void MouseControl(bool* buttons, float xChange, float yChange);
	virtual void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset);
	virtual void Update();
	inline Camera* GetCamera() { return m_Camera; };
	void InvertPitch();
	glm::mat4 CalculateViewMatrix();

private:
	void CalculateFront();

protected:
	Camera* m_Camera;

	float m_MoveSpeed;
	float m_TurnSpeed;
	float m_SpeedBoost = 4.0f;

	float m_MouseDeltaX = 0.0f;
	float m_MouseDeltaY = 0.0f;

};
