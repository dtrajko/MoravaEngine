#pragma once

#include "CommonValues.h"

#include <stdio.h>


class Window
{
public:
	Window();
	Window(GLint windowWidth, GLint windowHeight, const char* windowTitle);
	~Window();

	int Initialize();
	inline GLFWwindow* GetHandler() { return glfwWindow; };
	inline unsigned int GetBufferWidth() { return bufferWidth; };
	inline unsigned int GetBufferHeight() { return bufferHeight; };
	bool GetShouldClose() { return glfwWindowShouldClose(glfwWindow); };
	void SetShouldClose(bool shouldClose);
	bool* getKeys() { return keys; };
	bool* getMouseButtons() { return buttons; };
	bool* getKeysPrev() { return keys_prev; }; // previous states of keys
	bool* getMouseButtonsPrev() { return buttons_prev; }; // previos states of mouse buttons
	float getXChange();
	float getYChange();
	inline float GetMouseX() const { return m_MouseX; };
	inline float GetMouseY() const { return m_MouseY; };
	float getXMouseScrollOffset();
	float getYMouseScrollOffset();
	void SwapBuffers() { glfwSwapBuffers(glfwWindow); };
	void SetVSync(bool enabled);
	bool IsVSync() const;
	void SetCursorDisabled();
	void SetCursorNormal();

	bool IsMouseButtonClicked(int mouseButton);
	bool IsMouseButtonReleased(int mouseButton);

private:
	GLFWwindow* glfwWindow;

	const char* m_Title;
	GLint width;
	GLint height;
	unsigned int bufferWidth;
	unsigned int bufferHeight;

	bool keys[1024];
	bool buttons[32];

	bool keys_prev[1024];
	bool buttons_prev[32];

	static int m_ActionPrev;

	float m_MouseX;
	float m_MouseY;
	GLfloat lastX;
	GLfloat lastY;
	float xChange;
	float yChange;
	float xChangeReset;
	float yChangeReset;
	float xMouseScrollOffset;
	float yMouseScrollOffset;
	bool mouseFirstMoved;
	bool mouseCursorAboveWindow;
	bool m_VSync;
	float m_CursorIgnoreLimit;

	void CreateCallbacks();

	static void handleKeys(GLFWwindow* window, int key, int code, int action, int mode);
	static void handleMouse(GLFWwindow* window, double xPos, double yPos);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void cursorEnterCallback(GLFWwindow* window, int entered);
	static void windowSizeCallback(GLFWwindow* window, int width, int height);
	static void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

};
