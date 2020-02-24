#include "Camera.h"

#include "glm/glm.hpp"
#include "glm/gtx/compatibility.hpp"


Camera::Camera()
	: Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f, 2.0f, 0.1f)
{
}

Camera::Camera(glm::vec3 startPosition, glm::vec3 startUp, GLfloat startYaw, GLfloat startPitch,
	GLfloat startMoveSpeed, GLfloat startTurnSpeed)
{
	m_Position = startPosition;
	m_WorldUp = startUp;
	m_Yaw = startYaw;
	m_Pitch = startPitch;
	m_Front = glm::vec3(0.0f, 0.0f, -1.0f);

	m_MoveSpeed = startMoveSpeed;
	m_TurnSpeed = startTurnSpeed;

	update();
}

void Camera::keyControl(bool* keys, GLfloat deltaTime)
{
	GLfloat velocity = m_MoveSpeed * deltaTime;

	if (keys[GLFW_KEY_LEFT_SHIFT])
	{
		velocity *= m_SpeedBoost;
	}

	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
	{
		m_Position += m_Front * velocity;
	}
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
	{
		m_Position -= m_Front * velocity;
	}
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		m_Position -= m_Right * velocity;
	}
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		m_Position += m_Right * velocity;
	}
	if (keys[GLFW_KEY_Q])
	{
		m_Position -= m_Up * velocity;
	}
	if (keys[GLFW_KEY_E] || keys[GLFW_KEY_SPACE])
	{
		m_Position += m_Up * velocity;
	}

	if (keys[GLFW_KEY_L])
	{
		printf("Camera Position X: %.2f Y: %.2f Z: %.2f\n", m_Position.x, m_Position.y, m_Position.z);
		printf("Camera Direction X: %.2f Y: %.2f Z: %.2f\n", m_Front.x, m_Front.y, m_Front.z);
	}
}

void Camera::mouseControl(bool* buttons, GLfloat xChange, GLfloat yChange)
{
	if (buttons[GLFW_MOUSE_BUTTON_RIGHT])
	{
		m_Yaw += xChange * m_TurnSpeed;
		m_Pitch += yChange * m_TurnSpeed;

		update();
	}
}

void Camera::mouseScrollControl(bool* keys, GLfloat deltaTime, double xOffset, double yOffset)
{
	GLfloat velocity = m_MoveSpeed * (float)yOffset;

	if (keys[GLFW_KEY_LEFT_SHIFT])
	{
		velocity *= m_SpeedBoost;
	}

	if (velocity)
	{
		m_Position += m_Front * velocity;
	}
}

glm::vec3 Camera::getCameraPosition()
{
	return m_Position;
}

glm::vec3 Camera::getCameraDirection()
{
	return glm::normalize(m_Front);
}

glm::mat4 Camera::CalculateViewMatrix()
{
	glm::mat4 viewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
	return viewMatrix;
}

void Camera::update()
{
	m_Front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	m_Front.y = sin(glm::radians(m_Pitch));
	m_Front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	m_Front = glm::normalize(m_Front);

	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

Camera::~Camera()
{
}
