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
	void KeyControl(bool* keys, GLfloat deltaTime);
	void MouseControl(bool* buttons, GLfloat xChange, GLfloat yChange);
	void MouseScrollControl(bool* keys, GLfloat deltaTime, float xOffset, float yOffset);
	void SetPosition(glm::vec3 position);
	inline glm::vec3 GetPosition() const { return m_Position; };
	inline glm::vec3 GetDirection() const { return glm::normalize(m_Front); };
	inline float GetPitch() const { return m_Pitch; };
	inline float GetYaw() const { return m_Yaw; };
	inline glm::vec3 GetFront() const { return m_Front; };
	inline glm::vec3 GetUp() const { return m_Up; };
	void InvertPitch();
	glm::mat4 CalculateViewMatrix();
	~Camera();

private:
	void Update();

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
