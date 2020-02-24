#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>


class Camera
{

public:
	Camera();
	Camera(glm::vec3 startPosition, glm::vec3 startUp, GLfloat startYaw, GLfloat startPitch, 
		GLfloat startMoveSpeed, GLfloat startTurnSpeed);
	void keyControl(bool* keys, GLfloat deltaTime);
	void mouseControl(bool* buttons, GLfloat xChange, GLfloat yChange);
	void mouseScrollControl(bool* keys, GLfloat deltaTime, double xOffset, double yOffset);
	glm::vec3 getCameraPosition();
	glm::vec3 getCameraDirection();
	glm::mat4 CalculateViewMatrix();
	~Camera();

private:
	void update();

private:
	glm::vec3 m_Position;
	glm::vec3 m_Front;
	glm::vec3 m_Up;
	glm::vec3 m_Right;
	glm::vec3 m_WorldUp;

	GLfloat m_Yaw = 0.0f;
	GLfloat m_Pitch = 0.0f;
	GLfloat m_Roll = 0.0f; // not used

	GLfloat m_MoveSpeed;
	GLfloat m_TurnSpeed;
	GLfloat m_SpeedBoost = 4.0f;

};
