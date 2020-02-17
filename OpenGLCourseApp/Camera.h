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
	glm::vec3 getCameraPosition();
	glm::mat4 CalculateViewMatrix();
	~Camera();

private:
	void update();

private:
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	GLfloat yaw = 0.0f;
	GLfloat pitch = 0.0f;
	GLfloat roll = 0.0f; // not used

	GLfloat moveSpeed;
	GLfloat turnSpeed;

};
