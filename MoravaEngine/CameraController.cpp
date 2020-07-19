#include "CameraController.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include "glm/gtx/compatibility.hpp"
#include "glm/gtc/matrix_transform.hpp"


CameraController::CameraController()
	: CameraController(nullptr, 2.0f, 0.1f)
{
}

CameraController::CameraController(Camera* camera, float moveSpeed, float turnSpeed)
{
	m_Camera = camera;

	m_MoveSpeed = moveSpeed;
	m_TurnSpeed = turnSpeed;

	Update();
}

void CameraController::KeyControl(bool* keys, float deltaTime)
{
	// Don't move camera when using Ctrl+S or Ctrl+D in Editor
	if (keys[GLFW_KEY_LEFT_CONTROL]) return;

	float velocity = m_MoveSpeed * deltaTime;

	if (keys[GLFW_KEY_LEFT_SHIFT])
	{
		velocity *= m_SpeedBoost;
	}

	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetFront() * velocity);
	}
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetFront() * velocity);
	}
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetRight() * velocity);
	}
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetRight() * velocity);
	}
	if (keys[GLFW_KEY_Q])
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetUp() * velocity);
	}
	if (keys[GLFW_KEY_E] || keys[GLFW_KEY_SPACE])
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetUp() * velocity);
	}

	if (keys[GLFW_KEY_L])
	{
		printf("CameraController::KeyControl Position [ %.2ff, %.2ff, %.2ff ]\n", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
		printf("CameraController::KeyControl Front    [ %.2ff, %.2ff, %.2ff ]\n", m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
	}
}

void CameraController::MouseControl(bool* buttons, float xChange, float yChange)
{
	if (buttons[GLFW_MOUSE_BUTTON_RIGHT])
	{
		m_Camera->SetYaw(m_Camera->GetYaw() + xChange * m_TurnSpeed);
		m_Camera->SetPitch(m_Camera->GetPitch() + yChange * m_TurnSpeed);

		Update();
	}
}

void CameraController::MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset)
{
	if (abs(yOffset) < 0.1f || abs(yOffset) > 10.0f)
		return;

	GLfloat velocity = m_MoveSpeed * yOffset;

	if (keys[GLFW_KEY_LEFT_SHIFT])
	{
		velocity *= m_SpeedBoost;
	}

	m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetFront() * velocity);
}

void CameraController::Update()
{
	m_Camera->Update();
}

CameraController::~CameraController()
{
}
