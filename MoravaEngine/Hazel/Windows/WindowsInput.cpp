#include "WindowsInput.h"

#include "../Core/HazelWindow.h"

#include <GLFW/glfw3.h>


Input* Input::s_Instance = new WindowsInput();

bool WindowsInput::IsKeyPressedImpl(int keycode, GLFWwindow* window)
{
	auto state = glfwGetKey(window, keycode);
	return state == GLFW_PRESS || state == GLFW_REPEAT;
}
bool WindowsInput::IsMouseButtonPressedImpl(int button, GLFWwindow* window)
{
	auto state = glfwGetMouseButton(window, button);
	return state == GLFW_PRESS;
}

bool WindowsInput::IsMouseButtonReleasedImpl(int button, GLFWwindow* window)
{
	auto state = glfwGetMouseButton(window, button);
	return state == GLFW_RELEASE;
}

std::pair<float, float> WindowsInput::GetMousePositionImpl(GLFWwindow* window)
{
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	return { (float)xpos, (float)ypos };
}

float WindowsInput::GetMouseXImpl(GLFWwindow* window)
{
	auto [x, y] = GetMousePositionImpl(window);
	return x;
}

float WindowsInput::GetMouseYImpl(GLFWwindow* window)
{
	auto [x, y] = GetMousePositionImpl(window);
	return y;
}
