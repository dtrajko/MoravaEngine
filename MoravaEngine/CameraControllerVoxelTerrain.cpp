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
	m_CameraRig = glm::vec3(-5.0f, 2.0f, 0.0f);
}

void CameraControllerVoxelTerrain::KeyControl(bool* keys, float deltaTime)
{
	if (keys[GLFW_KEY_L])
	{
		printf("CameraControllerVoxelTerrain::KeyControl Position [ %.2ff, %.2ff, %.2ff ]\n",
			m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
		printf("CameraControllerVoxelTerrain::KeyControl Front    [ %.2ff, %.2ff, %.2ff ]\n",
			m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
	}
}

void CameraControllerVoxelTerrain::MouseControl(bool* buttons, float xChange, float yChange)
{
	// CameraController::MouseControl(buttons, xChange, yChange);
}

void CameraControllerVoxelTerrain::MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset)
{
	CameraController::MouseScrollControl(keys, deltaTime, xOffset, yOffset);
}

void CameraControllerVoxelTerrain::Update()
{
	CameraController::Update();

	m_Camera->SetYaw(-m_Player->GetRotation().y);
	m_Camera->SetPitch(m_Player->GetRotation().x);

	glm::vec3 cameraRigInFrontDirection = m_Player->GetFront() * m_CameraRig;
	m_Camera->SetPosition(m_Player->GetPosition() + cameraRigInFrontDirection);

	printf("CameraControllerVoxelTerrain::Update PlayerFront [ %.2ff %.2ff %.2ff ] Player Position [ %.2ff %.2ff %.2ff ]\n",
		m_Player->GetFront().x, m_Player->GetFront().y, m_Player->GetFront().z,
		m_Player->GetPosition().x, m_Player->GetPosition().y, m_Player->GetPosition().z);

	printf("CameraControllerVoxelTerrain::Update CameraRig [ %.2ff %.2ff %.2ff ] CameraRigInFrontDirection [ %.2ff %.2ff %.2ff ]\n",
		m_CameraRig.x, m_CameraRig.y, m_CameraRig.z,
		cameraRigInFrontDirection.x, cameraRigInFrontDirection.y, cameraRigInFrontDirection.z);

	printf("CameraControllerVoxelTerrain::Update Camera Position [ %.2ff %.2ff %.2ff ]\n",
		m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
}

CameraControllerVoxelTerrain::~CameraControllerVoxelTerrain()
{
}
