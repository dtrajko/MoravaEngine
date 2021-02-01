#pragma once

#include "CommonValues.h"
#include "Hazel/Events/Event.h"

#include <stdio.h>
#include <functional>
#include <string>


struct WindowProps
{
	std::string Title;
	uint32_t Width;
	uint32_t Height;

	WindowProps(const std::string& title = "3D Graphics Engine (C++ / OpenGL)",
		uint32_t width = 1280,
		uint32_t height = 720)
		: Title(title), Width(width), Height(height)
	{
	}
};

class Window
{
	/**** BEGIN Window Hazel version - a platform independent Window interface ****/
public:
	using EventCallbackFn = std::function<void(Event&)>;

	virtual ~Window() {};

	virtual void OnUpdate() = 0;

	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;

	// Window attributes
	virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	virtual void SetVSync(bool enabled) = 0;
	virtual bool IsVSync() const = 0;
	virtual void SetInputMode(bool cursorEnabled) = 0;

	static Window* Create(const WindowProps& props = WindowProps());

	/**** END Window Hazel version - a platform independent Window interface ****/

	// Methods used by MoravaEngine classes
	virtual GLFWwindow* GetHandle() { return nullptr; };
	virtual bool* getKeys() { return nullptr; };
	virtual bool* getMouseButtons() { return nullptr; };
	virtual bool IsMouseButtonClicked(int mouseButton) { return false; };
	virtual float GetMouseX() const { return 0.0f; };
	virtual float GetMouseY() const { return 0.0f; };
	virtual float getXChange() { return 0.0f; };
	virtual float getYChange() { return 0.0f; };
	virtual float getXMouseScrollOffset() { return 0.0f; };
	virtual float getYMouseScrollOffset() { return 0.0f; };
	virtual void SetShouldClose(bool shouldClose) {};
	virtual void SetCursorDisabled() {};
	virtual void SetCursorNormal() {};
	virtual bool GetShouldClose() { return false; };

	virtual void SetEventLogging(bool enabled) = 0;
	virtual const bool GetEventLogging() const = 0;

	virtual void SetTitle(std::string title) = 0;
	virtual std::string GetTitle() = 0;

};
