#pragma once

#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>


class Window
{
public:
	Window();
	Window(GLint windowWidth, GLint windowHeight);
	int Initialize();
	inline GLfloat GetBufferWidth() { return (GLfloat)bufferWidth; };
	inline GLfloat GetBufferHeight() { return (GLfloat)bufferHeight; };
	bool GetShouldClose() { return glfwWindowShouldClose(mainWindow); };
	void SwapBuffers() { glfwSwapBuffers(mainWindow); };
	~Window();

private:
	GLFWwindow* mainWindow;
	GLint width;
	GLint height;
	GLsizei bufferWidth;
	GLsizei bufferHeight;

};
