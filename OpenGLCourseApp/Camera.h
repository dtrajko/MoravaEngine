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
	void mouseScrollControl(bool* keys, GLfloat deltaTime, float xOffset, float yOffset);
	void SetPosition(glm::vec3 position);
	inline glm::vec3 getPosition() const { return m_Position; };
	inline glm::vec3 getDirection() const { return glm::normalize(m_Front); };
	inline float getPitch() const { return m_Pitch; };
	void InvertPitch();
	glm::mat4 CalculateViewMatrix();
	glm::mat4 CalculateViewMatrixStrife();
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
