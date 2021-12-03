#include "Camera.h"

#include "Core/CommonValues.h"


Camera::Camera() : Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f) {}

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
	: H2M::SceneCameraH2M()
{
	m_Position = position;
	m_Yaw = yaw;
	m_Pitch = pitch;
	m_PerspectiveFOV = glm::radians(45.0f);
	m_AspectRatio = 1.778f;

	m_WorldUp = up;
	m_Front = glm::vec3(0.0f, 0.0f, -1.0f);

	UpdateView();
}

Camera::Camera(glm::vec3 position, float yaw, float pitch, float fovDegrees, float aspectRatio, float moveSpeed, float turnSpeed)
{
	m_Position = position;
	m_Yaw = yaw;
	m_Pitch = pitch;
	m_PerspectiveFOV = glm::radians(fovDegrees);
	m_AspectRatio = aspectRatio;

	m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	m_Front = glm::vec3(0.0f, 0.0f, -1.0f);

	UpdateView();
}

Camera::~Camera() {}

void Camera::UpdateProjection()
{
	m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
	m_ProjectionMatrix = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
}

void Camera::UpdateView()
{
	m_ViewMatrix = glm::lookAt(m_Position, m_Position + glm::normalize(m_Front), m_Up);
}

void Camera::OnUpdate(H2M::TimestepH2M ts)
{
	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));

	UpdateView();
}

void Camera::OnEvent(H2M::EventH2M& e)
{
	H2M::EventDispatcherH2M dispatcher(e);
	dispatcher.Dispatch<H2M::MouseScrolledEventH2M>(H2M_BIND_EVENT_FN(Camera::OnMouseScroll));
}

void Camera::SetViewportSize(float width, float height)
{
	m_ViewportWidth = width;
	m_ViewportHeight = height;

	UpdateProjection();
}

void Camera::SetPitch(float pitch)
{
	// preventing the invertion of orientation
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch <= -89.0f) pitch = -89.0f;

	m_Pitch = pitch;
}

glm::mat4& Camera::GetViewMatrix()
{
	m_ViewMatrix = glm::lookAt(m_Position, m_Position + glm::normalize(m_Front), m_Up);
	return m_ViewMatrix;
}
