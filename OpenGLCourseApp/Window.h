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
	bool* getKeys() { return keys; };
	bool* getMouseButtons() { return buttons; };
	GLfloat getXChange();
	GLfloat getYChange();
	double getXMouseScrollOffset();
	double getYMouseScrollOffset();
	void SwapBuffers() { glfwSwapBuffers(mainWindow); };
	~Window();

private:
	GLFWwindow* mainWindow;

	GLint width;
	GLint height;
	GLsizei bufferWidth;
	GLsizei bufferHeight;

	bool keys[1024];
	bool buttons[32];

	GLfloat lastX;
	GLfloat lastY;
	GLfloat xChange;
	GLfloat yChange;
	double xMouseScrollOffset;
	double yMouseScrollOffset;
	bool mouseFirstMoved;
	bool mouseCursorAboveWindow;

	void CreateCallbacks();

	static void handleKeys(GLFWwindow* window, int key, int code, int action, int mode);
	static void handleMouse(GLFWwindow* window, double xPos, double yPos);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void cursorEnterCallback(GLFWwindow* window, int entered);
	static void windowSizeCallback(GLFWwindow* window, int width, int height);
	static void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

};
