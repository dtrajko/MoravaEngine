#pragma once

#include "Window.h"


class Application
{
public:
	static Application* Get();
	static Window* GetWindow();
	static void SetWindow(Window* window);

private:
	static Application* s_Instance;
	static Window* s_Window;

};
