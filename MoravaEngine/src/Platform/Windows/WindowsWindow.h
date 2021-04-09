#pragma once

#include "Hazel/Renderer/RendererContext.h"

#include "Core/Window.h"


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
	virtual inline GLFWwindow* GetHandle() override { return glfwWindow; };
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

	bool* getKeysPrev() { return keys_prev; }; // previous states of keys
	bool* getMouseButtonsPrev() { return buttons_prev; }; // previos states of mouse buttons
	// void SwapBuffers();

	bool IsMouseButtonReleased(int mouseButton);

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
	static void CharCallback(GLFWwindow* window, unsigned int codepoint);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void CursorEnterCallback(GLFWwindow* window, int entered);
	static void WindowSizeCallback(GLFWwindow* window, int width, int height);
	static void WindowCloseCallback(GLFWwindow* window);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void SetCallbacks(); // Obsolete after adding GLFW callbacks to Init() method
	void SetCallbacksHazelDev();

	virtual inline void SetEventLogging(bool enabled) override { m_EventLoggingEnabled = enabled; }
	virtual inline const bool GetEventLogging() const override { return m_EventLoggingEnabled; }

	virtual void SetTitle(std::string title) override;
	virtual std::string GetTitle() override;

	virtual Hazel::Ref<Hazel::RendererContext> GetRenderContext() override { return m_RendererContext; }

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

	bool m_EventLoggingEnabled;

	Hazel::Ref<Hazel::RendererContext> m_RendererContext;

};
