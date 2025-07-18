#pragma once

#include "H2M/Core/Events/EventH2M.h"
#include "H2M/Renderer/RendererContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanSwapChainH2M.h"

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
	virtual std::pair<float, float> GetWindowPos() const = 0;

	// Window attributes
	virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	virtual void SetVSync(bool enabled) = 0;
	virtual bool IsVSync() const = 0;

	virtual void Maximize() = 0;

	virtual const std::string& GetTitle() const = 0;
	virtual void SetTitle(std::string title) = 0;

	virtual void SetInputMode(bool cursorEnabled) = 0;

	static Window* Create(const WindowSpecification& windowSpecification = WindowSpecification());

	/**** END Window Hazel version - a platform independent Window interface ****/

	/**** BEGIN DirectX 11 methods ***/
	virtual void SetHWND(HWND hwnd) = 0;
	virtual HWND GetHWND() = 0;

	// Events
	virtual void OnCreate() = 0;
	virtual void OnDestroy() = 0;
	virtual void OnFocus() = 0;
	virtual void OnKillFocus() = 0;
	virtual void OnSize() = 0;

	virtual bool IsInFocus() = 0;
	virtual void SetInFocus(bool inFocus) = 0;
	/**** END DirectX 11 methods ***/

	virtual void OnUpdate() = 0;

	// Methods used by MoravaEngine classes
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
	virtual bool GetShouldClose() = 0;

	virtual void SetEventLogging(bool enabled) = 0;
	virtual const bool GetEventLogging() const = 0;

	virtual H2M::RefH2M<H2M::RendererContextH2M> GetRenderContext() = 0;
	virtual H2M::VulkanSwapChainH2M& GetSwapChain() = 0;

};
