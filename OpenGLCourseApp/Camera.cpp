#include "Camera.h"

Camera::Camera()
	: Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f, 2.0f, 0.1f)
{
}

Camera::Camera(glm::vec3 startPosition, glm::vec3 startUp, GLfloat startYaw, GLfloat startPitch,
	GLfloat startMoveSpeed, GLfloat startTurnSpeed)
{
	position = startPosition;
	worldUp = startUp;
	yaw = startYaw;
	pitch = startPitch;
	front = glm::vec3(0.0f, 0.0f, -1.0f);

	moveSpeed = startMoveSpeed;
	turnSpeed = startTurnSpeed;

	update();
}

void Camera::keyControl(bool* keys, GLfloat deltaTime)
{
	GLfloat velocity = moveSpeed * deltaTime;

	if (keys[GLFW_KEY_W])
	{
		position += front * velocity;
	}
	if (keys[GLFW_KEY_S])
	{
		position -= front * velocity;
	}
	if (keys[GLFW_KEY_A])
	{
		position -= right * velocity;
	}
	if (keys[GLFW_KEY_D])
	{
		position += right * velocity;
	}
	if (keys[GLFW_KEY_Q])
	{
		position -= up * velocity;
	}
	if (keys[GLFW_KEY_E])
	{
		position += up * velocity;
	}
}

void Camera::mouseControl(GLfloat xChange, GLfloat yChange)
{
	yaw += xChange * turnSpeed;
	pitch += yChange * turnSpeed;

	update();
}

glm::mat4 Camera::CalculateViewMatrix()
{
	glm::mat4 viewMatrix = glm::lookAt(position, position + front, up);
	return viewMatrix;
}

void Camera::update()
{
	printf("Pitch: %.2f, Yaw: %.2f\n", pitch, yaw);

	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);

	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));
}

Camera::~Camera()
{
}

