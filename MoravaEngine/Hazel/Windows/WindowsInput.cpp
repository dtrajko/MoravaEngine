#include "WindowsInput.h"

#include "../../Application.h"

#include <GLFW/glfw3.h>


Input* Input::s_Instance = new WindowsInput();

bool WindowsInput::IsKeyPressedImpl(int keycode)
{
	GLFWwindow* window = Application::Get()->GetWindow()->GetHandler();
	auto state = glfwGetKey(window, keycode);
	return state == GLFW_PRESS || state == GLFW_REPEAT;
}
bool WindowsInput::IsMouseButtonPressedImpl(int button)
{
	GLFWwindow* window = Application::Get()->GetWindow()->GetHandler();
	auto state = glfwGetMouseButton(window, button);
	return state == GLFW_PRESS;
}

bool WindowsInput::IsMouseButtonReleasedImpl(int button)
{
	GLFWwindow* window = Application::Get()->GetWindow()->GetHandler();
	auto state = glfwGetMouseButton(window, button);
	return state == GLFW_RELEASE;
}

std::pair<float, float> WindowsInput::GetMousePositionImpl()
{
	GLFWwindow* window = Application::Get()->GetWindow()->GetHandler();
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	return { (float)xpos, (float)ypos };
}

float WindowsInput::GetMouseXImpl()
{
	auto [x, y] = GetMousePositionImpl();
	return x;
}

float WindowsInput::GetMouseYImpl()
{
	auto [x, y] = GetMousePositionImpl();
	return y;
}
