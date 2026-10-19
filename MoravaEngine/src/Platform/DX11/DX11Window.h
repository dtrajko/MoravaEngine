#pragma once

#include "Core/Window.h"
#include "Core/CommonStructs.h"
#include "Platform/DX11/DX11InputListener.h"

#include "H2M/Renderer/RendererContextH2M.h"


/**
 * The application's window for the DirectX 11 renderer: a Win32 window (Windows only). Its input comes from
 * DX11InputSystem, not from GLFW: the GLFW parts of the Window interface (the GLFW handle, the key / button / mouse
 * state the Morava scenes read) are empty here.
 */
class DX11Window : public Window, public DX11InputListener
{
public:
	DX11Window(const WindowSpecification& windowSpecification);
	virtual ~DX11Window();

	virtual void ProcessEvents() override;
	virtual void SwapBuffers() override;

	inline uint32_t GetWidth() const override { return m_Data.Width; };
	inline uint32_t GetHeight() const override { return m_Data.Height; };

	virtual std::pair<uint32_t, uint32_t> GetSize() const override { return { m_Data.Width, m_Data.Height }; }
	virtual std::pair<int, int> GetWindowPos() const override;

	// Window attributes
	virtual void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
	virtual void SetVSync(bool enabled) override { m_Data.VSync = enabled; }
	virtual bool IsVSync() const override { return m_Data.VSync; }

	virtual void Maximize() override {}

	virtual const std::string& GetTitle() const override { return m_Data.Title; }
	virtual void SetTitle(std::string title) override { m_Data.Title = title; }

	virtual void SetInputMode(bool cursorEnabled) override {}

	virtual H2M::RefH2M<H2M::RendererContextH2M> GetRenderContext() override { return m_RendererContext; }
#if defined(GRAPHICS_API_VULKAN)
	virtual H2M::VulkanSwapChainH2M& GetSwapChain() override { return m_SwapChain; } // not used: a build with Vulkan too
#endif

	virtual void OnUpdate() override;

	// The Win32 window
	// A handler that sees the window's messages first (the ImGui layer's: ImGui_ImplWin32_WndProcHandler); a result other
	// than 0: the message is handled. nullptr: none.
	using MessageHandler = LRESULT(*)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static void SetMessageHandler(MessageHandler handler);

	void SetHWND(HWND hwnd) { m_HWND = hwnd; }
	HWND GetHWND() { return m_HWND; }
	bool Broadcast();

	bool IsRunning() { return m_IsRunning; }
	RECT GetClientWindowRect();
	RECT GetSizeScreen();
	bool Release();

	// Events (from the window procedure)
	void OnCreate();
	void OnDestroy();
	void OnFocus();
	void OnKillFocus();
	void OnSize();

	// Inherited via DX11InputListener
	virtual void OnKeyDown(int key) override {}
	virtual void OnKeyUp(int key) override {}

	virtual void OnMouseMove(const glm::vec2& mousePosDelta, const glm::vec2& mousePosAbs) override {}

	virtual void OnLeftMouseDown(const glm::vec2& mousePos) override {}
	virtual void OnRightMouseDown(const glm::vec2& mousePos) override {}

	virtual void OnLeftMouseUp(const glm::vec2& mousePos) override {}
	virtual void OnRightMouseUp(const glm::vec2& mousePos) override {}

	virtual bool IsInFocus() override { return m_InFocus; }
	virtual void SetInFocus(bool inFocus) override { m_InFocus = inFocus; }

	// The GLFW parts of the interface: no GLFW window, no input through it
	virtual inline GLFWwindow* GetHandle() override { return nullptr; };
	virtual inline GLFWwindow* GetNativeWindow() override { return nullptr; };
	virtual bool* getKeys() override { return m_Keys; };
	virtual bool* getMouseButtons() override { return m_Buttons; };
	virtual bool IsMouseButtonClicked(int mouseButton) override { return false; }
	virtual inline float GetMouseX() const override { return 0.0f; };
	virtual inline float GetMouseY() const override { return 0.0f; };
	virtual float getXChange() override { return 0.0f; }
	virtual float getYChange() override { return 0.0f; }
	virtual float getXMouseScrollOffset() override { return 0.0f; }
	virtual float getYMouseScrollOffset() override { return 0.0f; }
	virtual void SetShouldClose(bool shouldClose) override { m_IsRunning = !shouldClose; }
	virtual void SetCursorDisabled() override {}
	virtual void SetCursorNormal() override {}
	virtual void SetCursorCaptured(bool captured) override {}
	virtual bool GetShouldClose() override { return !m_IsRunning; }

	virtual inline void SetEventLogging(bool enabled) override { m_EventLoggingEnabled = enabled; }
	virtual inline const bool GetEventLogging() const override { return m_EventLoggingEnabled; }

private:
	void Init();

private:
	WindowSpecification m_Specification;

	struct WindowData
	{
		std::string Title;
		uint32_t Width, Height;
		bool VSync;

		EventCallbackFn EventCallback;
	};

	WindowData m_Data;
	H2M::RefH2M<H2M::RendererContextH2M> m_RendererContext;
#if defined(GRAPHICS_API_VULKAN)
	H2M::VulkanSwapChainH2M m_SwapChain; // not used: the interface returns a reference to one
#endif

	HWND m_HWND = nullptr; // windef.h

	bool m_IsInitialized = false;
	bool m_IsRunning = true;

	bool m_Keys[1024] = {};
	bool m_Buttons[32] = {};

	bool m_EventLoggingEnabled = false;

	bool m_InFocus = false;

	EventCooldown m_ResizeViewport = { 0.0f, 0.2f };

};
