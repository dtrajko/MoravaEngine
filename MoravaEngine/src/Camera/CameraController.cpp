#include "Camera/CameraController.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "ImGui/ImGuiWrapper.h"

#include <cstdio>


CameraController::CameraController(H2M::CameraH2M* camera, float aspectRatio, float moveSpeed, float turnSpeed)
{
	m_Camera = camera;
	m_Camera->SetAspectRatio(aspectRatio);
	m_MoveSpeed = moveSpeed;
	m_TurnSpeed = turnSpeed;
}

void CameraController::KeyControl(bool* keys, float deltaTime)
{
	// Don't move the camera when using Ctrl+S or Ctrl+D in Editor
	if (Input::IsKeyPressed(KeyH2M::LeftControl)) return;

	// Move the camera only the viewport accepts events, i.e. in focus or hovered (mouse over)
	if (!ImGuiWrapper::CanViewportReceiveEvents()) return;

	float velocity = m_MoveSpeed * deltaTime;

	if (Input::IsKeyPressed(KeyH2M::LeftShift))
	{
		velocity *= m_SpeedBoost;
	}
	if (Input::IsKeyPressed(KeyH2M::W) || Input::IsKeyPressed(KeyH2M::Up))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetFront() * velocity);
	}
	if (Input::IsKeyPressed(KeyH2M::S) || Input::IsKeyPressed(KeyH2M::Down))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetFront() * velocity);
	}
	if (Input::IsKeyPressed(KeyH2M::A) || Input::IsKeyPressed(KeyH2M::Left))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetRight() * velocity);
	}
	if (Input::IsKeyPressed(KeyH2M::D) || Input::IsKeyPressed(KeyH2M::Right))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetRight() * velocity);
	}
	if (Input::IsKeyPressed(KeyH2M::Q))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetUp() * velocity);
	}
	if (Input::IsKeyPressed(KeyH2M::E) || Input::IsKeyPressed(KeyH2M::Space))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetUp() * velocity);
	}

	if (Input::IsKeyPressed(KeyH2M::L))
	{
		Log::GetLogger()->debug("CameraController GLFW_KEY_L {0}, keys[GLFW_KEY_L] {1}", GLFW_KEY_L, keys[GLFW_KEY_L]);
		Log::GetLogger()->debug("CameraController::KeyControl Position [ {0}, {1}, {2} ]", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
		Log::GetLogger()->debug("CameraController::KeyControl Front    [ {0}, {1}, {2} ]", m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
	}
}

void CameraController::MouseControl(bool* buttons, float xChange, float yChange)
{
	if (Input::IsMouseButtonPressed(MouseH2M::ButtonRight))
	{
		m_Camera->SetYaw(m_Camera->GetYaw() + xChange * m_TurnSpeed);
		m_Camera->SetPitch(m_Camera->GetPitch() - yChange * m_TurnSpeed);
	}
}

void CameraController::MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset)
{
	if (abs(yOffset) < 0.1f || abs(yOffset) > 10.0f)
		return;

	GLfloat velocity = m_MoveSpeed * yOffset;

	if (Input::IsKeyPressed(KeyH2M::LeftShift))
	{
		velocity *= m_SpeedBoost;
	}

	m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetFront() * velocity);
}

void CameraController::Update()
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

	CalculateFront();
}

void CameraController::OnEvent(H2M::EventH2M& e)
{
}

void CameraController::InvertPitch()
{
	float pitch = m_Camera->GetPitch();
	m_Camera->SetPitch(-pitch);

	CalculateFront();
}

void CameraController::InvertRoll()
{
	glm::vec3 newWorldUp = -m_Camera->GetWorldUp();
	m_Camera->SetWorldUp(newWorldUp);
	m_Camera->OnUpdate(0);
}

void CameraController::OnResize(uint32_t width, uint32_t height)
{
	// TODO (void H2M::OrthographicCameraController::OnResize(float width, float height))
	m_Camera->SetAspectRatio((float)width / (float)height);
}

void CameraController::CalculateFront()
{
	float pitch = m_Camera->GetPitch();
	float yaw = m_Camera->GetYaw();
	glm::vec3 front = glm::vec3(0.0f);
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = -sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);
	m_Camera->SetFront(front);
}

CameraController::~CameraController()
{
}
