#pragma once

#include "Window.h"


class WindowsWindow : public Window
{
	/**** BEGIN Window Hazel version - a platform independent Window interface ****/
public:
	using EventCallbackFn = std::function<void(Event&)>;

	virtual ~WindowsWindow() override;

	virtual void OnUpdate() override;

	virtual uint32_t GetWidth() const { return bufferWidth; };
	virtual uint32_t GetHeight() const { return bufferHeight; };

	// Window attributes
	virtual void SetEventCallback(const EventCallbackFn& callback) override;
	//	virtual void SetVSync(bool enabled) = 0;
	//	virtual bool IsVSync() const = 0;
	virtual void SetVSync(bool enabled);
	virtual bool IsVSync() const;

	static Window* Create(const WindowProps& props = WindowProps()) {};

	virtual void Init(const WindowProps& props);
	virtual void Shutdown();

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
	WindowsWindow();
	WindowsWindow(GLint windowWidth, GLint windowHeight, const char* windowTitle);

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

	int Initialize();
	inline unsigned int GetBufferWidth() { return bufferWidth; };
	inline unsigned int GetBufferHeight() { return bufferHeight; };
	bool GetShouldClose() { return glfwWindowShouldClose(glfwWindow); };
	bool* getKeysPrev() { return keys_prev; }; // previous states of keys
	bool* getMouseButtonsPrev() { return buttons_prev; }; // previos states of mouse buttons
	void SwapBuffers() { glfwSwapBuffers(glfwWindow); };

	bool IsMouseButtonReleased(int mouseButton);

	void CreateCallbacks();

	static void handleKeys(GLFWwindow* window, int key, int code, int action, int mode);
	static void handleMouse(GLFWwindow* window, double xPos, double yPos);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void cursorEnterCallback(GLFWwindow* window, int entered);
	static void windowSizeCallback(GLFWwindow* window, int width, int height);
	static void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

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

};
