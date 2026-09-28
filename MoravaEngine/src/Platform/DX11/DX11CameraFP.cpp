#include "DX11CameraFP.h"

#include "DX11InputSystem.h"
#include "DX11TestLayer.h"

#include "Core/Application.h"
#include "Core/Timer.h"
#include "ImGui/ImGuiWrapper.h"
#include "Platform/Windows/WindowsWindow.h"

#include "imgui.h"


DX11CameraFP::DX11CameraFP() : DX11CameraFP(glm::mat4(1.0f))
{
}

DX11CameraFP::DX11CameraFP(glm::mat4 projection) : H2M::CameraH2M(projection)
{
	DX11InputSystem::Get()->AddListener(this);

	m_Position = glm::vec3(0.0f, 6.0f, 20.0f);

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

void DX11CameraFP::OnUpdate(H2M::TimestepH2M ts)
{
	// Always consume the raw motion, so movement made before a drag starts is not applied
	glm::vec2 rawMouseDelta = DX11InputSystem::Get()->ConsumeRawMouseDelta();

	if (m_CameraRotationEnabled)
	{
		// The button-up can be missed (e.g. the window lost focus during the drag): check the physical button
		if (!(::GetAsyncKeyState(VK_RBUTTON) & 0x8000))
		{
			EndRotation();
		}
		else
		{
			// The view is lookAt(+m_Front) with a left-handed projection, so the visible direction is -m_Front:
			// decreasing yaw turns right, decreasing pitch looks down
			m_Yaw -= rawMouseDelta.x * m_MouseSensitivity;
			m_Pitch -= rawMouseDelta.y * m_MouseSensitivity;
		}
	}

	UpdateView();
}

void DX11CameraFP::BeginRotation()
{
	HWND hwnd = Application::Get()->GetWindow()->GetHWND();

	// Lock the cursor where the drag started (it reappears there) and keep receiving mouse input outside the window
	POINT cursorPos = {};
	::GetCursorPos(&cursorPos);
	RECT clipRect = { cursorPos.x, cursorPos.y, cursorPos.x + 1, cursorPos.y + 1 };
	::ClipCursor(&clipRect);
	::SetCapture(hwnd);
	::ShowCursor(FALSE);

	// ImGui still sees the (hidden) cursor: keep panels from reacting to it while rotating
	if (ImGui::GetCurrentContext() != nullptr)
	{
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
	}

	DX11InputSystem::Get()->ConsumeRawMouseDelta();
	m_CameraRotationEnabled = true;
}

void DX11CameraFP::EndRotation()
{
	m_CameraRotationEnabled = false;

	::ClipCursor(nullptr);
	::ReleaseCapture();
	::ShowCursor(TRUE);

	if (ImGui::GetCurrentContext() != nullptr)
	{
		ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}
}

//	DX11CameraFP* DX11CameraFP::Get()
//	{
//		static DX11CameraFP camera;
//		return &camera;
//	}

void DX11CameraFP::OnEvent(H2M::EventH2M& e)
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
	// Rotation uses raw mouse motion (see OnUpdate): the cursor position is locked while rotating
}

// DX11InputListener API
void DX11CameraFP::OnLeftMouseDown(const glm::vec2& mousePos)
{
}

// DX11InputListener API
void DX11CameraFP::OnRightMouseDown(const glm::vec2& mousePos)
{
	if (!m_Enabled || m_CameraRotationEnabled) return;

	// Only start rotating over the scene (DX11Renderer's "Viewport" window), not over other ImGui panels
	if (!ImGuiWrapper::CanViewportReceiveEvents()) return;

	BeginRotation();
}

// DX11InputListener API
void DX11CameraFP::OnLeftMouseUp(const glm::vec2& mousePos)
{
}

// DX11InputListener API
void DX11CameraFP::OnRightMouseUp(const glm::vec2& mousePos)
{
	if (m_CameraRotationEnabled)
	{
		EndRotation();
	}
}
