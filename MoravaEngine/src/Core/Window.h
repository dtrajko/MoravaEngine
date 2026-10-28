#pragma once

#include "H2M/Core/Events/EventH2M.h"
#include "H2M/Renderer/RendererContextH2M.h"
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanSwapChainH2M.h"
#endif

#include "Core/CommonValues.h"

#include <stdio.h>
#include <functional>
#include <string>


struct WindowSpecification
{
	std::string Title = "Untitled";
	uint32_t Width = 1280;
	uint32_t Height = 720;
	bool Decorated = true;
	bool Fullscreen = false;
	bool VSync = true;

	WindowSpecification(std::string title = "Untitled", uint32_t width = 1280, uint32_t height = 720)
	: Title(title), Width(width), Height(height) {}

};

class Window
{
	/**** BEGIN Window Hazel version - a platform independent Window interface ****/
public:
	using EventCallbackFn = std::function<void(H2M::EventH2M&)>;

	virtual ~Window() {};

	virtual void ProcessEvents() = 0;
	virtual void SwapBuffers() = 0;

	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;

	virtual std::pair<uint32_t, uint32_t> GetSize() const = 0;
	virtual std::pair<int, int> GetWindowPos() const = 0;

	// Window attributes
	virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	virtual void SetVSync(bool enabled) = 0;
	virtual bool IsVSync() const = 0;

	virtual void Maximize() = 0;

	virtual const std::string& GetTitle() const = 0;
	virtual void SetTitle(std::string title) = 0;

	virtual void SetInputMode(bool cursorEnabled) = 0;

	// The window of the current renderer API: a GLFWWindow (OpenGL, Vulkan; every platform) or a DX11Window (Windows)
	static Window* Create(const WindowSpecification& windowSpecification = WindowSpecification());

	/**** END Window Hazel version - a platform independent Window interface ****/

	// The window has the keyboard's input
	virtual bool IsInFocus() = 0;
	virtual void SetInFocus(bool inFocus) = 0;

	virtual void OnUpdate() = 0;

	// Methods used by MoravaEngine classes
	// The GLFW window (nullptr: the window isn't a GLFW one, as DirectX 11's)
	virtual GLFWwindow* GetHandle() = 0;
	virtual GLFWwindow* GetNativeWindow() = 0; // an alias/synonim of GetHandle
	virtual bool* getKeys() = 0;
	virtual bool* getMouseButtons() = 0;
	virtual bool IsMouseButtonClicked(int mouseButton) = 0;
	virtual float GetMouseX() const = 0;
	virtual float GetMouseY() const = 0;
	virtual float getXChange() = 0;
	virtual float getYChange() = 0;
	virtual float getXMouseScrollOffset() = 0;
	virtual float getYMouseScrollOffset() = 0;
	virtual void SetShouldClose(bool shouldClose) = 0;
	virtual void SetCursorDisabled() = 0;
	virtual void SetCursorNormal() = 0;
	virtual void SetCursorCaptured(bool captured) = 0; // hide and lock the cursor, read raw mouse motion (e.g. while rotating the camera)
	virtual bool GetShouldClose() = 0;

	// The window covers the monitor it is on (at the monitor's resolution, no title bar); false: back to its place and
	// size before (maximized again if it was)
	virtual void SetFullscreen(bool fullscreen) = 0;
	virtual bool IsFullscreen() = 0;

	virtual void SetEventLogging(bool enabled) = 0;
	virtual const bool GetEventLogging() const = 0;

	virtual H2M::RefH2M<H2M::RendererContextH2M> GetRenderContext() = 0;
#if defined(GRAPHICS_API_VULKAN)
	virtual H2M::VulkanSwapChainH2M& GetSwapChain() = 0;
#endif

};
