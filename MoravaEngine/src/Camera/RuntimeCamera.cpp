#include "Camera/RuntimeCamera.h"

#include "Core/Application.h"
#include "Core/MousePicker.h"
#include "Core/Timer.h"


RuntimeCamera::RuntimeCamera()
{
	m_ProjectionType = H2M::CameraH2M::ProjectionType::Perspective;
    m_CameraController = CameraController(this, 1.778f, 2.0f, 0.1f);
}

RuntimeCamera::RuntimeCamera(glm::vec3 position, float yaw, float pitch, float fovDegrees, float aspectRatio, float moveSpeed, float turnSpeed)
{
	m_Position = position;
	m_Yaw = yaw;
	m_Pitch = pitch;
	m_PerspectiveFOV = glm::radians(fovDegrees);
	m_AspectRatio = aspectRatio;
	m_ProjectionType = H2M::CameraH2M::ProjectionType::Perspective;

	m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	m_Front = glm::vec3(0.0f, 0.0f, -1.0f);

	m_CameraController = CameraController(this, aspectRatio, moveSpeed, turnSpeed);
}

RuntimeCamera::~RuntimeCamera()
{
}

void RuntimeCamera::UpdateProjection()
{
	m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
	m_ProjectionMatrix = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
}

void RuntimeCamera::UpdateView()
{
	m_ViewMatrix = glm::lookAt(m_Position, m_Position + glm::normalize(m_Front), m_Up);
}

void RuntimeCamera::OnUpdate(H2M::TimestepH2M ts)
{
	m_CameraController.Update();

	MousePicker::Get()->Update(
		(int)Application::Get()->GetWindow()->GetMouseX(), (int)Application::Get()->GetWindow()->GetMouseY(),
		0, 0, (int)Application::Get()->GetWindow()->GetWidth(), (int)Application::Get()->GetWindow()->GetHeight(),
		m_ProjectionMatrix, m_ViewMatrix);

	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));

	UpdateView();
}

void RuntimeCamera::OnEvent(H2M::EventH2M& e)
{
	m_CameraController.OnEvent(e);

	H2M::EventDispatcherH2M dispatcher(e);
	dispatcher.Dispatch<H2M::MouseScrolledEventH2M>(H2M_BIND_EVENT_FN(RuntimeCamera::OnMouseScroll));
}

void RuntimeCamera::SetViewportSize(float width, float height)
{
	m_ViewportWidth = width;
	m_ViewportHeight = height;

	UpdateProjection();
}

void RuntimeCamera::SetPitch(float pitch)
{
	// preventing the invertion of orientation
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch <= -89.0f) pitch = -89.0f;

	m_Pitch = pitch;
}

glm::mat4& RuntimeCamera::GetViewMatrix()
{
	m_ViewMatrix = glm::lookAt(m_Position, m_Position + glm::normalize(m_Front), m_Up);
	return m_ViewMatrix;
}
