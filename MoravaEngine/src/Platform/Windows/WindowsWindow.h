#pragma once

#include "Hazel/Renderer/RendererContext.h"

#include "Platform/DX11/DX11InputListener.h"

#include "Core/Window.h"


class WindowsWindow : public Window, public DX11InputListener
{
	/**** BEGIN Window Hazel version - a platform independent Window interface ****/
public:
	WindowsWindow(const WindowProps& props);
	virtual ~WindowsWindow();

	virtual void ProcessEvents() override;
	virtual void SwapBuffers() override;

	inline uint32_t GetWidth() const override { return m_Data.Width; };
	inline uint32_t GetHeight() const override { return m_Data.Height; };

	virtual std::pair<uint32_t, uint32_t> GetSize() const override { return { m_Data.Width, m_Data.Height }; }
	virtual std::pair<float, float> GetWindowPos() const override;

	// Window attributes
	virtual void SetEventCallback(const EventCallbackFn& callback) override;
	virtual void SetVSync(bool enabled) override;
	virtual bool IsVSync() const override;

	virtual void Maximize() override;

	virtual const std::string& GetTitle() const override { return m_Data.Title; }
	virtual void SetTitle(std::string title) override;

	inline void* GetNativeWindow() const { return m_GLFW_Window; }

	virtual Hazel::Ref<Hazel::RendererContext> GetRenderContext() override { return m_RendererContext; }

	void SetInputMode(bool cursorEnabled) override;

	void OnUpdate() override;

private:
	virtual void Init(const WindowProps& props);
	virtual void Shutdown();

	void InitGLFW(const WindowProps& props);
	void InitDX11(const WindowProps& props);
	/**** END Window Hazel version - a platform independent Window interface ****/

	/**** BEGIN DirectX 11 methods ****/
public:
	virtual void SetHWND(HWND hwnd) override;
	virtual HWND GetHWND() { return m_HWND; }
	bool Broadcast();

	bool IsRunning();
	RECT GetClientWindowRect();
	RECT GetSizeScreen();
	bool Release();

	// Events
	virtual void OnCreate() override;
	virtual void OnDestroy() override;
	virtual void OnFocus() override;
	virtual void OnKillFocus() override;
	virtual void OnSize() override;

	// Inherited via DX11InputListener
	virtual void OnKeyDown(int key) override;
	virtual void OnKeyUp(int key) override;

	virtual void OnMouseMove(const glm::vec2& mousePos) override;

	virtual void OnLeftMouseDown(const glm::vec2& mousePos) override;
	virtual void OnRightMouseDown(const glm::vec2& mousePos) override;

	virtual void OnLeftMouseUp(const glm::vec2& mousePos) override;
	virtual void OnRightMouseUp(const glm::vec2& mousePos) override;

	virtual bool IsInFocus() override { return m_InFocus; }
	virtual void SetInFocus(bool inFocus) override { m_InFocus = inFocus; }
	/**** END DirectX 11 methods ****/

public:
	virtual inline GLFWwindow* GetHandle() override { return m_GLFW_Window; };
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
	virtual bool GetShouldClose() override;

	bool* getKeysPrev() { return keys_prev; }; // previous states of keys
	bool* getMouseButtonsPrev() { return buttons_prev; }; // previos states of mouse buttons

	bool IsMouseButtonReleased(int mouseButton);

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
	static void CharCallback(GLFWwindow* window, unsigned int codepoint);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void CursorEnterCallback(GLFWwindow* window, int entered);
	static void WindowSizeCallback(GLFWwindow* window, int width, int height);
	static void WindowCloseCallback(GLFWwindow* window);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	// void SetCallbacks(); // Obsolete after adding GLFW callbacks to Init() method
	void SetCallbacksHazelDev();

	virtual inline void SetEventLogging(bool enabled) override { m_EventLoggingEnabled = enabled; }
	virtual inline const bool GetEventLogging() const override { return m_EventLoggingEnabled; }

private:
	/**** BEGIN Window Hazel version - a platform independent Window interface ****/

	GLFWwindow* m_GLFW_Window;

	struct WindowData
	{
		std::string Title;
		uint32_t Width, Height;
		bool VSync;

		EventCallbackFn EventCallback;
	};

	WindowData m_Data;
	Hazel::Ref<Hazel::RendererContext> m_RendererContext;

	/**** END Window Hazel version - a platform independent Window interface ****/

	HWND m_HWND; // windef.h

	bool m_IsInitialized = false;
	bool m_IsRunning = true;

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

	bool m_InFocus;

};
