#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Events/Event.h"

#include <functional>


struct WindowProps
{
	std::string Title;
	uint32_t Width;
	uint32_t Height;

	WindowProps(const std::string& title = "Hazel Engine",
			    uint32_t width = 1280,
			    uint32_t height = 720)
		: Title(title), Width(width), Height(height)
	{
	}
};

// Interface representing a desktop system based Window
class HazelWindow
{
public:
	using EventCallbackFn = std::function<void(Event&)>;

	virtual ~HazelWindow() {}

	virtual void OnUpdate() = 0;

	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;

	// Window attributes
	virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	virtual void SetVSync(bool enabled) = 0;
	virtual bool IsVSync() const = 0;
	virtual void SetInputMode(bool cursorEnabled) = 0;

	virtual void* GetNativeWindow() const = 0;

	static HazelWindow* Create(const WindowProps& props = WindowProps());
};
