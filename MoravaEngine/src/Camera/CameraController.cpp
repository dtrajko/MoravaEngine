#include "CameraController.h"

#include "../../Log.h"
#include "../../Input.h"
#include "../../Application.h"
#include "../../Timer.h"
#include "../../ImGuiWrapper.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include "glm/gtx/compatibility.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cstdio>


CameraController::CameraController()
{
	m_Camera = new Camera(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);
	m_Camera->SetAspectRatio(16 / 9.0f);
	m_MoveSpeed = 2.0f;
	m_TurnSpeed = 0.1f;
}

CameraController::CameraController(Hazel::HazelCamera* camera, float aspectRatio, float moveSpeed, float turnSpeed)
{
	m_Camera = camera;
	m_Camera->SetAspectRatio(aspectRatio);
	m_MoveSpeed = moveSpeed;
	m_TurnSpeed = turnSpeed;
}

void CameraController::KeyControl(bool* keys, float deltaTime)
{
	// Don't move the camera when using Ctrl+S or Ctrl+D in Editor
	if (Input::IsKeyPressed(Key::LeftControl)) return;

	// Move the camera only the viewport accepts events, i.e. in focus or hovered (mouse over)
	if (!ImGuiWrapper::CanViewportReceiveEvents()) return;

	float velocity = m_MoveSpeed * deltaTime;

	if (Input::IsKeyPressed(Key::LeftShift))
	{
		velocity *= m_SpeedBoost;
	}
	if (Input::IsKeyPressed(Key::W) || Input::IsKeyPressed(Key::Up))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetFront() * velocity);
	}
	if (Input::IsKeyPressed(Key::S) || Input::IsKeyPressed(Key::Down))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetFront() * velocity);
	}
	if (Input::IsKeyPressed(Key::A) || Input::IsKeyPressed(Key::Left))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetRight() * velocity);
	}
	if (Input::IsKeyPressed(Key::D) || Input::IsKeyPressed(Key::Right))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetRight() * velocity);
	}
	if (Input::IsKeyPressed(Key::Q))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetUp() * velocity);
	}
	if (Input::IsKeyPressed(Key::E) || Input::IsKeyPressed(Key::Space))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetUp() * velocity);
	}

	if (Input::IsKeyPressed(Key::L))
	{
		Log::GetLogger()->debug("CameraController GLFW_KEY_L {0}, keys[GLFW_KEY_L] {1}", GLFW_KEY_L, keys[GLFW_KEY_L]);
		Log::GetLogger()->debug("CameraController::KeyControl Position [ {0}, {1}, {2} ]", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
		Log::GetLogger()->debug("CameraController::KeyControl Front    [ {0}, {1}, {2} ]", m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
	}
}

void CameraController::MouseControl(bool* buttons, float xChange, float yChange)
{
	if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
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

	if (Input::IsKeyPressed(Key::LeftShift))
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

void CameraController::OnEvent(Event& e)
{
}

void CameraController::InvertPitch()
{
	float pitch = m_Camera->GetPitch();
	m_Camera->SetPitch(-pitch);
}

void CameraController::OnResize(uint32_t width, uint32_t height)
{
	// TODO (void Hazel::OrthographicCameraController::OnResize(float width, float height))
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
