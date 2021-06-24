#include "DX11CameraFP.h"

#include "DX11InputSystem.h"

#include "Core/Application.h"
#include "Core/Timer.h"


DX11CameraFP::DX11CameraFP() : DX11CameraFP(glm::mat4(1.0f))
{
}

DX11CameraFP::DX11CameraFP(glm::mat4 projection)
	: m_Projection(projection)
{
	DX11InputSystem::Get()->AddListener(this);
}

DX11CameraFP::~DX11CameraFP()
{
}

void DX11CameraFP::Update()
{
}

DX11CameraFP* DX11CameraFP::Get()
{
	static DX11CameraFP camera;
	return &camera;
}

void DX11CameraFP::OnEvent(Event& e)
{
}

// Hazel camera API
void DX11CameraFP::SetViewportSize(float width, float height)
{
}

// Hazel camera API
void DX11CameraFP::SetProjectionMatrix(glm::mat4 projection)
{
}

// DX11InputListener API
void DX11CameraFP::OnKeyDown(int key)
{
	if (!Application::Get()->GetWindow()->IsInFocus()) return;

	float velocity = m_CameraSpeed * Timer::Get()->GetDeltaTime();

	if (key == 'W') // Forwards
	{
		m_CameraPosition -= m_CameraVectorFront * velocity;
	}
	if (key == 'S') // Backwards
	{
		m_CameraPosition += m_CameraVectorFront * velocity;
	}
	if (key == 'A') // Left
	{
		m_CameraPosition -= m_CameraVectorRight * velocity;
	}
	if (key == 'D') // Right
	{
		m_CameraPosition += m_CameraVectorRight * velocity;
	}
	if (key == 'Q') // Down
	{
		m_CameraPosition -= m_CameraVectorUp * velocity;
	}
	if (key == 'E') // Up
	{
		m_CameraPosition += m_CameraVectorUp * velocity;
	}
}

// DX11InputListener API
void DX11CameraFP::OnKeyUp(int key)
{
}

// DX11InputListener API
void DX11CameraFP::OnMouseMove(const DX11Point& deltaMousePos)
{
	if (!Application::Get()->GetWindow()->IsInFocus()) return;

	float turnVelocity = m_CameraTurnSpeed * Timer::Get()->GetDeltaTime();

	m_CameraYaw -= deltaMousePos.m_X * turnVelocity;
	m_CameraPitch -= deltaMousePos.m_Y * turnVelocity;
}

// DX11InputListener API
void DX11CameraFP::OnLeftMouseDown(const DX11Point& deltaMousePos)
{
}

// DX11InputListener API
void DX11CameraFP::OnRightMouseDown(const DX11Point& deltaMousePos)
{
}

// DX11InputListener API
void DX11CameraFP::OnLeftMouseUp(const DX11Point& deltaMousePos)
{
}

// DX11InputListener API
void DX11CameraFP::OnRightMouseUp(const DX11Point& deltaMousePos)
{
}

// Moved from DX11Renderer
glm::mat4& DX11CameraFP::CalculateViewMatrix()
{
	// Based on Core/Camera calculations

	// preventing the invertion of orientation
	if (m_CameraPitch > 89.0f) m_CameraPitch = 89.0f;
	if (m_CameraPitch <= -89.0f) m_CameraPitch = -89.0f;

	glm::vec3 worldUpVector = glm::vec3(0.0f, 1.0f, 0.0f);

	m_CameraVectorFront.x = cos(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));
	m_CameraVectorFront.y = -sin(glm::radians(m_CameraPitch));
	m_CameraVectorFront.z = sin(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));
	m_CameraVectorFront = glm::normalize(m_CameraVectorFront);

	m_CameraVectorRight = glm::normalize(glm::cross(m_CameraVectorFront, worldUpVector));
	m_CameraVectorUp = glm::normalize(glm::cross(m_CameraVectorRight, m_CameraVectorFront));

	m_View = glm::lookAt(m_CameraPosition, m_CameraPosition + glm::normalize(m_CameraVectorFront), m_CameraVectorUp);

	return m_View;
}
