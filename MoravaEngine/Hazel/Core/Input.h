#pragma once

#include "Base.h"


class Input
{
public:
	inline static bool IsKeyPressed(int keycode, GLFWwindow* window) { return s_Instance->IsKeyPressedImpl(keycode, window); }
	inline static bool IsMouseButtonPressed(int button, GLFWwindow* window) { return s_Instance->IsMouseButtonPressedImpl(button, window); };
	inline static bool IsMouseButtonReleased(int button, GLFWwindow* window) { return s_Instance->IsMouseButtonReleasedImpl(button, window); };
	inline static std::pair<float, float> GetMousePosition(GLFWwindow* window) { return s_Instance->GetMousePositionImpl(window); };
	inline static float GetMouseX(GLFWwindow* window) { return s_Instance->GetMouseXImpl(window); };
	inline static float GetMouseY(GLFWwindow* window) { return s_Instance->GetMouseYImpl(window); };

protected:
	virtual bool IsKeyPressedImpl(int keycode, GLFWwindow* window) = 0;
	virtual bool IsMouseButtonPressedImpl(int button, GLFWwindow* window) = 0;
	virtual bool IsMouseButtonReleasedImpl(int button, GLFWwindow* window) = 0;
	virtual std::pair<float, float> GetMousePositionImpl(GLFWwindow* window) = 0;
	virtual float GetMouseXImpl(GLFWwindow* window) = 0;
	virtual float GetMouseYImpl(GLFWwindow* window) = 0;

private:
	static Input* s_Instance;

};
