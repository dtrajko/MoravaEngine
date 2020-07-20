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
	m_CameraRig = glm::vec3(-8.0f, 2.0f, 0.0f);
}

void CameraControllerVoxelTerrain::KeyControl(bool* keys, float deltaTime)
{
	if (keys[GLFW_KEY_L])
	{
		printf("CameraControllerVoxelTerrain::KeyControl Camera Position [ %.2ff, %.2ff, %.2ff ]\n",
			m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
		printf("CameraControllerVoxelTerrain::KeyControl Camera Front    [ %.2ff, %.2ff, %.2ff ]\n",
			m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
	}
}

void CameraControllerVoxelTerrain::MouseControl(bool* buttons, float xChange, float yChange)
{
	if (buttons[GLFW_MOUSE_BUTTON_RIGHT])
	{
		// float oldPitch = m_Camera->GetPitch();
		// float newPitch = oldPitch - yChange * m_CameraPitchSpeed;
		// m_CameraRig.x -= (oldPitch - newPitch) * m_CameraPitchSpeed;
		// m_CameraRig.y -= (oldPitch - newPitch) * m_CameraPitchSpeed;
		// m_Camera->SetPitch(newPitch);
		// m_Camera->Update();
	}
}

void CameraControllerVoxelTerrain::MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset)
{
	// CameraController::MouseScrollControl(keys, deltaTime, xOffset, yOffset);
}

void CameraControllerVoxelTerrain::Update()
{
	m_Camera->SetYaw(-m_Player->GetRotation().y);
	// m_Camera->SetPitch(m_Player->GetRotation().x);

	glm::vec3 newCameraPosition = m_Player->GetPosition() + (m_Player->GetFront() * m_CameraRig.x) + glm::vec3(0.0f, m_CameraRig.y, 0.0f);
	m_Camera->SetPosition(newCameraPosition);

	printf("CameraControllerVoxelTerrain::Update PlayerFront [ %.2ff %.2ff %.2ff ] Player Position [ %.2ff %.2ff %.2ff ]\n",
		m_Player->GetFront().x, m_Player->GetFront().y, m_Player->GetFront().z,
		m_Player->GetPosition().x, m_Player->GetPosition().y, m_Player->GetPosition().z);

	printf("CameraControllerVoxelTerrain::Update Camera Front [ %.2ff %.2ff %.2ff ] Camera Position [ %.2ff %.2ff %.2ff ]\n",
		m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z,
		m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);

	printf("CameraControllerVoxelTerrain::Update Camera Pitch [ %.2ff ] Camera Yaw [ %.2ff ]\n",
		m_Camera->GetPitch(), m_Camera->GetYaw());

	printf("CameraControllerVoxelTerrain::Update CameraRig [ %.2ff %.2ff %.2ff ] newCameraPosition [ %.2ff %.2ff %.2ff ]\n",
		m_CameraRig.x, m_CameraRig.y, m_CameraRig.z,
		newCameraPosition.x, newCameraPosition.y, newCameraPosition.z);

	m_Camera->Update();
}

CameraControllerVoxelTerrain::~CameraControllerVoxelTerrain()
{
}
