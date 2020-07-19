#pragma once

#include "Camera.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>


class CameraController
{

public:
	CameraController();
	CameraController(Camera* camera, float moveSpeed, float turnSpeed);
	void KeyControl(bool* keys, GLfloat deltaTime);
	void MouseControl(bool* buttons, GLfloat xChange, GLfloat yChange);
	void MouseScrollControl(bool* keys, GLfloat deltaTime, float xOffset, float yOffset);
	inline Camera* GetCamera() { return m_Camera; };
	~CameraController();

private:
	void Update();

private:
	Camera* m_Camera;

	GLfloat m_MoveSpeed;
	GLfloat m_TurnSpeed;
	GLfloat m_SpeedBoost = 4.0f;

	float m_MouseDeltaX = 0.0f;
	float m_MouseDeltaY = 0.0f;

};
