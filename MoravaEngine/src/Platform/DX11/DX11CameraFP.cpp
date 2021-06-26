#include "DX11CameraFP.h"

#include "DX11InputSystem.h"
#include "DX11TestLayer.h"

#include "Core/Application.h"
#include "Core/Timer.h"
#include "Platform/Windows/WindowsWindow.h"


DX11CameraFP::DX11CameraFP() : DX11CameraFP(glm::mat4(1.0f))
{
}

DX11CameraFP::DX11CameraFP(glm::mat4 projection) : Hazel::HazelCamera(projection)
{
	DX11InputSystem::Get()->AddListener(this);

	m_Position = glm::vec3(0.0f, 0.0f, 4.0f);

	m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	m_Front = glm::vec3(0.0f, 0.0f, 1.0f);

	m_Pitch = 0.0f;
	m_Yaw = 90.0f;

	m_PerspectiveFOV = glm::radians(60.0f);

	UpdateView();
}

DX11CameraFP::~DX11CameraFP()
{
}

void DX11CameraFP::OnUpdate(Hazel::Timestep ts)
{
	UpdateView();
}

//	DX11CameraFP* DX11CameraFP::Get()
//	{
//		static DX11CameraFP camera;
//		return &camera;
//	}

void DX11CameraFP::OnEvent(Event& e)
{
}

// Hazel camera API
void DX11CameraFP::SetViewportSize(float width, float height)
{
	m_ViewportWidth = width;
	m_ViewportHeight = height;

	UpdateProjection();
}

void DX11CameraFP::UpdateProjection()
{
	// Projection matrix (perspective)
	m_ProjectionMatrix = glm::perspectiveFovLH(m_PerspectiveFOV, m_ViewportWidth, m_ViewportHeight, m_PerspectiveNear, m_PerspectiveFar);
}

void DX11CameraFP::UpdateView()
{
	// Based on Core/Camera calculations

	// preventing the invertion of orientation
	if (m_Pitch > 89.0f) m_Pitch = 89.0f;
	if (m_Pitch <= -89.0f) m_Pitch = -89.0f;

	// Log::GetLogger()->info("Yaw: {0}, Pitch: {1}, Position: [{2}, {3}, {4}]", m_CameraYaw, m_CameraPitch, m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z);

	m_Front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	m_Front.y = -sin(glm::radians(m_Pitch));
	m_Front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	m_Front = glm::normalize(m_Front);

	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));

	m_ViewMatrix = glm::lookAt(m_Position, m_Position + glm::normalize(m_Front), m_Up);
}

// DX11InputListener API
void DX11CameraFP::OnKeyDown(int key)
{
	if (!m_Enabled) return;

	float velocity = m_MoveSpeed * Timer::Get()->GetDeltaTime();

	if (key == VK_LSHIFT)
	{
		m_SpeedBoostEnabled = true;
	}

	if (m_SpeedBoostEnabled)
	{
		velocity *= m_SpeedBoost;
	}

	// Log::GetLogger()->info("velocity: {0}", velocity);

	if (key == 'W') // Forwards
	{
		m_Position -= m_Front * velocity;
	}
	if (key == 'S') // Backwards
	{
		m_Position += m_Front * velocity;
	}
	if (key == 'A') // Left
	{
		m_Position -= m_Right * velocity;
	}
	if (key == 'D') // Right
	{
		m_Position += m_Right * velocity;
	}
	if (key == 'Q') // Down
	{
		m_Position -= m_Up * velocity;
	}
	if (key == 'E') // Up
	{
		m_Position += m_Up * velocity;
	}
}

// DX11InputListener API
void DX11CameraFP::OnKeyUp(int key)
{
	if (key == VK_LSHIFT)
	{
		m_SpeedBoostEnabled = false;
	}
}

// DX11InputListener API
void DX11CameraFP::OnMouseMove(const glm::vec2& mousePosDelta, const glm::vec2& mousePosAbs)
{
	if (!m_Enabled) return;

	WindowsWindow* windowsWindow = (WindowsWindow*)Application::Get()->GetWindow();
	RECT windowRECT = windowsWindow->GetClientWindowRect();

	// Log::GetLogger()->info("Window RECT: Left: {0}, Right: {1}, Top: {2}, Bottom: {3}", windowRECT.left, windowRECT.right, windowRECT.top, windowRECT.bottom);
	// Log::GetLogger()->info("mousePosDelta: {0}x{1}, mousePosAbs: [{2}x{3}])", mousePosDelta.x, mousePosDelta.y, mousePosAbs.x, mousePosAbs.y);

	// A dirty hack to prevent sudden jumps in delta mouse position
	// if (abs(mousePosDelta.x) < 4.0f && abs(mousePosDelta.y) < 4.0f) {}

	float turnVelocity = m_TurnSpeed * Timer::Get()->GetDeltaTime();

	// absolute mouse position (new)
	glm::vec2 centralMousePos = glm::vec2(
		windowRECT.left + ((windowRECT.right - windowRECT.left) / 2.0f),
		windowRECT.top + ((windowRECT.bottom - windowRECT.top) / 2.0f));

	// delta mouse position (old)
	// m_Yaw -= mousePosDelta.x * turnVelocity;
	// m_Pitch -= mousePosDelta.y * turnVelocity;

	m_Yaw -= (mousePosAbs.x - centralMousePos.x) * turnVelocity;
	m_Pitch -= (mousePosAbs.y - centralMousePos.y) * turnVelocity;

	if (windowsWindow->IsInFocus() && DX11InputSystem::Get()->IsMouseCursorAboveViewport()) {}

	DX11InputSystem::Get()->SetCursorPosition(centralMousePos);
}

// DX11InputListener API
void DX11CameraFP::OnLeftMouseDown(const glm::vec2& mousePos)
{
}

// DX11InputListener API
void DX11CameraFP::OnRightMouseDown(const glm::vec2& mousePos)
{
}

// DX11InputListener API
void DX11CameraFP::OnLeftMouseUp(const glm::vec2& mousePos)
{
}

// DX11InputListener API
void DX11CameraFP::OnRightMouseUp(const glm::vec2& mousePos)
{
}
