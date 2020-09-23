#pragma once

#include "Window.h"


class WindowsWindow : public Window
{
	/**** BEGIN Window Hazel version - a platform independent Window interface ****/
public:
	WindowsWindow(const WindowProps& props);
	virtual ~WindowsWindow();

	void OnUpdate() override;

	inline uint32_t GetWidth() const override { return m_Data.Width; };
	inline uint32_t GetHeight() const override { return m_Data.Height; };

	// Window attributes
	void SetEventCallback(const EventCallbackFn& callback) override;
	void SetVSync(bool enabled) override;
	bool IsVSync() const override;
	void SetInputMode(bool cursorEnabled) override;

private:
	virtual void Init(const WindowProps& props);
	virtual void Shutdown();

	static Window* Create(const WindowProps& props = WindowProps()) {};

private:
	// GLFWwindow* m_Window;
	struct WindowData
	{
		std::string Title;
		uint32_t Width, Height;
		bool VSync;

		EventCallbackFn EventCallback;
	};

	WindowData m_Data;
	/**** END Window Hazel version - a platform independent Window interface ****/

public:
	virtual inline GLFWwindow* GetHandler() override { return glfwWindow; };
	virtual bool* getKeys() override { return keys; };
	virtual bool* getMouseButtons() override { return buttons; };
	virtual bool IsMouseButtonClicked(int mouseButton) override;
	virtual inline float GetMouseX() const override { return m_MouseX; };
	virtual inline float GetMouseY() const override { return m_MouseY; };
	virtual float getXChange() override;
	virtual float getYChange() override;
	virtual float getXMouseScrollOffset() override;
	virtual float getYMouseScrollOffset() override;
	virtual void SetShouldClose(bool shouldClose) override;
	virtual void SetCursorDisabled() override;
	virtual void SetCursorNormal() override;
	virtual bool GetShouldClose() override { return glfwWindowShouldClose(glfwWindow); };

	inline unsigned int GetBufferWidth() { return m_Data.Width; };
	inline unsigned int GetBufferHeight() { return m_Data.Height; };
	bool* getKeysPrev() { return keys_prev; }; // previous states of keys
	bool* getMouseButtonsPrev() { return buttons_prev; }; // previos states of mouse buttons
	// void SwapBuffers();

	bool IsMouseButtonReleased(int mouseButton);

	void CreateCallbacks();
	static void handleKeys(GLFWwindow* window, int key, int code, int action, int mode);
	static void handleMouse(GLFWwindow* window, double xPos, double yPos);
	static void handleChars(GLFWwindow* window, unsigned int keycode);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void cursorEnterCallback(GLFWwindow* window, int entered);
	static void windowSizeCallback(GLFWwindow* window, int width, int height);
	static void windowCloseCallback(GLFWwindow* window);
	static void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

private:
	GLFWwindow* glfwWindow;

	bool keys[1024];
	bool buttons[32];

	bool keys_prev[1024];
	bool buttons_prev[32];

	static int m_ActionPrev;

	float m_MouseX;
	float m_MouseY;
	float lastX;
	float lastY;
	float xChange;
	float yChange;
	float xChangeReset;
	float yChangeReset;
	float xMouseScrollOffset;
	float yMouseScrollOffset;
	bool mouseFirstMoved;
	bool mouseCursorAboveWindow;
	float m_CursorIgnoreLimit;

};
