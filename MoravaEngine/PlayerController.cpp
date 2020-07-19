#include "PlayerController.h"

#include <GLFW/glfw3.h>


PlayerController::PlayerController()
{
}

PlayerController::PlayerController(Player* player)
{
	m_Player = player;
}

PlayerController::~PlayerController()
{
}

void PlayerController::KeyControl(bool* keys, float deltaTime)
{
	// Set gravity
	m_Player->SetPosition(m_Player->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f) * m_Gravity);

	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
	{
		m_Player->SetPosition(m_Player->GetPosition() + glm::vec3(0.0f, 0.0f, -1.0f) * m_MoveSpeed);
	}
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
	{
		m_Player->SetPosition(m_Player->GetPosition() + glm::vec3(0.0f, 0.0f, 1.0f) * m_MoveSpeed);
	}
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		m_Player->SetPosition(m_Player->GetPosition() + glm::vec3(-1.0f, 0.0f, 0.0f) * m_MoveSpeed);
	}
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		m_Player->SetPosition(m_Player->GetPosition() + glm::vec3(1.0f, 0.0f, 0.0f) * m_MoveSpeed);
	}
	if (keys[GLFW_KEY_Q])
	{
		m_Player->SetPosition(m_Player->GetPosition() + glm::vec3(0.0f, -1.0f, 0.0f) * m_MoveSpeed);
	}
	if (keys[GLFW_KEY_E] || keys[GLFW_KEY_SPACE])
	{
		m_Player->SetPosition(m_Player->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f) * m_MoveSpeed);
	}
}

void PlayerController::MouseControl(bool* buttons, float xChange, float yChange)
{
}

void PlayerController::MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset)
{
}

void PlayerController::Update()
{
	m_Player->Update();
}
