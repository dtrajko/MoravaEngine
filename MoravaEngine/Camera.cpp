#include "Camera.h"


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/gtx/compatibility.hpp"
#include "glm/gtc/matrix_transform.hpp"


Camera::Camera() : Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f)
{
}

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
{
	m_Position = position;
	m_WorldUp = up;
	m_Yaw = yaw;
	m_Pitch = pitch;
	m_Roll = 0.0f;
	m_Front = glm::vec3(0.0f, 0.0f, -1.0f);

	Update();
}

void Camera::InvertPitch()
{
	m_Pitch = -m_Pitch;
	Update();
}

glm::mat4 Camera::CalculateViewMatrix()
{
	glm::mat4 viewMatrix = glm::lookAt(m_Position, m_Position + glm::normalize(m_Front), m_Up);
	return viewMatrix;
}

void Camera::SetFront()
{
	m_Front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	m_Front.y = sin(glm::radians(m_Pitch));
	m_Front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	m_Front = glm::normalize(m_Front);
}

void Camera::Update()
{
	SetFront();
	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void Camera::SetDirection(glm::vec3 direction)
{
	m_Pitch = asin(-direction.y) / glm::radians(1.0f);
	m_Yaw = atan2(direction.x, direction.z) / glm::radians(1.0f);
	m_Front = glm::normalize(direction);
}

Camera::~Camera()
{
}
