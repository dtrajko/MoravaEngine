#include "Camera/CameraControllerVoxelTerrain.h"


#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Timer.h"


CameraControllerVoxelTerrain::CameraControllerVoxelTerrain()
	: CameraControllerVoxelTerrain(nullptr, nullptr, 16 / 9.0f, 2.0f, 0.1f, 10.0f)
{
}

CameraControllerVoxelTerrain::CameraControllerVoxelTerrain(H2M::CameraH2M* camera, Player* player, float aspectRatio, float moveSpeed, float turnSpeed, float cameraPlayerDistance)
	: CameraController(camera, aspectRatio, moveSpeed, turnSpeed)
{
	m_Player = player;
	m_CameraPlayerDistance = cameraPlayerDistance;
	m_AngleAroundPlayer = 180.0f;
	m_PitchChangeSpeed = 0.1f;
	m_YawChangeSpeed = 0.03f;
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
	m_Buttons = buttons;
	m_xChange = xChange;
	m_yChange = yChange;
}

void CameraControllerVoxelTerrain::MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset)
{
	m_Keys = keys;
	m_xOffset = xOffset;
	m_yOffset = yOffset;
}

void CameraControllerVoxelTerrain::Update()
{
	KeyControl(Application::Get()->GetWindow()->getKeys(), Timer::Get()->GetDeltaTime());

	MouseControl(
		Application::Get()->GetWindow()->getMouseButtons(),
		Application::Get()->GetWindow()->getXChange(),
		Application::Get()->GetWindow()->getYChange());

	MouseScrollControl(
		Application::Get()->GetWindow()->getKeys(), Timer::Get()->GetDeltaTime(),
		Application::Get()->GetWindow()->getXMouseScrollOffset(),
		Application::Get()->GetWindow()->getYMouseScrollOffset());

	CalculateZoom(m_yOffset);
	CalculatePitch(m_Buttons, m_yChange);
	CalculateAngleAroundPlayer(m_Keys, m_Buttons, m_xChange);

	float horizontalDistance = CalculateHorizontalDistance();
	float verticalDistance = CalculateVerticalDistance();

	CalculateCameraPosition(horizontalDistance, verticalDistance);

	float yaw = 180.0f - (m_Player->GetRotation().y + m_AngleAroundPlayer);
	m_Camera->SetYaw(yaw);

	CalculateFront();
	m_Camera->OnUpdate(0);

	UpdateDebugInfo();
}

void CameraControllerVoxelTerrain::CalculateZoom(float yOffset)
{
	if (abs(m_yOffset) < 0.1f || abs(m_yOffset) > 10.0f)
		return;

	m_ZoomLevel = yOffset;
	m_CameraPlayerDistance -= m_ZoomLevel;
}

void CameraControllerVoxelTerrain::CalculatePitch(bool* buttons, float yChange)
{
	if (m_UnlockRotation || buttons[GLFW_MOUSE_BUTTON_RIGHT])
	{
		float pitchChange = yChange * m_PitchChangeSpeed;
		float pitch = m_Camera->GetPitch();
		pitch -= pitchChange;
		m_Camera->SetPitch(pitch);
	}
}

void CameraControllerVoxelTerrain::CalculateAngleAroundPlayer(bool* keys, bool* buttons, float xChange)
{
	if (buttons[GLFW_MOUSE_BUTTON_LEFT] && keys[GLFW_KEY_C])
	{
		float angleChange = xChange * m_YawChangeSpeed;
		m_AngleAroundPlayer -= angleChange;
	}
}

float CameraControllerVoxelTerrain::CalculateHorizontalDistance()
{
	float pitch = m_Camera->GetPitch();
	return m_CameraPlayerDistance * std::cos(glm::radians(pitch));
}

float CameraControllerVoxelTerrain::CalculateVerticalDistance()
{
	float pitch = m_Camera->GetPitch();
	return m_CameraPlayerDistance * std::sin(glm::radians(pitch));
}

void CameraControllerVoxelTerrain::CalculateCameraPosition(float horizontalDistance, float verticalDistance)
{
	m_Theta = m_Player->GetRotation().y + m_AngleAroundPlayer;
	float offsetX = horizontalDistance * -std::cos(glm::radians(m_Theta));
	float offsetZ = horizontalDistance * std::sin(glm::radians(m_Theta));

	glm::vec3 playerPosition = m_Player->GetPosition();
	glm::vec3 cameraPosition = m_Camera->GetPosition();

	glm::vec3 newCameraPosition = glm::vec3(cameraPosition);
	newCameraPosition.x = playerPosition.x - offsetX;
	newCameraPosition.z = playerPosition.z - offsetZ;
	newCameraPosition.y = playerPosition.y + verticalDistance;

	m_Camera->SetPosition(newCameraPosition);
}

void CameraControllerVoxelTerrain::UpdateDebugInfo()
{
	m_DebugPlayerPosition = m_Player->GetPosition();
	m_DebugCameraPosition = m_Camera->GetPosition();
	m_DebugPlayerFront = m_Player->GetFront();
	m_DebugCameraFront = m_Camera->GetFront();
	m_DebugCameraPitch = m_Camera->GetPitch();
	m_DebugCameraYaw = m_Camera->GetYaw();
	m_DebugAngleAroundPlayer = m_AngleAroundPlayer;
	m_DebugTheta = m_Theta;
}

void CameraControllerVoxelTerrain::InvertPitch()
{
	float pitch = m_Camera->GetPitch();
	m_Camera->SetPitch(-pitch);
	Update();
}

CameraControllerVoxelTerrain::~CameraControllerVoxelTerrain()
{
}
