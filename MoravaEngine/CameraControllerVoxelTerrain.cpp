#include "CameraControllerVoxelTerrain.h"

#include "CommonValues.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>


CameraControllerVoxelTerrain::CameraControllerVoxelTerrain()
	: CameraControllerVoxelTerrain(nullptr, nullptr, 2.0f, 0.1f)
{
}

CameraControllerVoxelTerrain::CameraControllerVoxelTerrain(Camera* camera, Player* player, float moveSpeed, float turnSpeed)
	: CameraController(camera, moveSpeed, turnSpeed)
{
	m_Player = player;
}

void CameraControllerVoxelTerrain::KeyControl(bool* keys, float deltaTime)
{
	if (keys[GLFW_KEY_L])
	{
		printf("CameraControllerVoxelTerrain::KeyControl Position [ %.2ff, %.2ff, %.2ff ]\n", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
		printf("CameraControllerVoxelTerrain::KeyControl Front    [ %.2ff, %.2ff, %.2ff ]\n", m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
	}
}

void CameraControllerVoxelTerrain::MouseControl(bool* buttons, float xChange, float yChange)
{
	CameraController::MouseControl(buttons, xChange, yChange);
}

void CameraControllerVoxelTerrain::MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset)
{
	CameraController::MouseScrollControl(keys, deltaTime, xOffset, yOffset);
}

void CameraControllerVoxelTerrain::Update()
{
	CameraController::Update();

	// glm::vec3 playerDirection = glm::eulerAngles(m_Player->GetRotation() / toRadians);
	// m_Camera->SetDirection(playerDirection);
	m_Camera->SetPosition(m_Player->GetPosition() + glm::vec3(0.0f, 2.0f, 10.0f));
}

CameraControllerVoxelTerrain::~CameraControllerVoxelTerrain()
{
}
