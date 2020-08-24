#pragma once

#include "../Core/Input.h"


class WindowsInput : public Input
{
protected:
	virtual bool IsKeyPressedImpl(int keycode, GLFWwindow* window) override;
	virtual bool IsMouseButtonPressedImpl(int button, GLFWwindow* window) override;
	virtual bool IsMouseButtonReleasedImpl(int button, GLFWwindow* window) override;
	virtual std::pair<float, float> GetMousePositionImpl(GLFWwindow* window) override;
	virtual float GetMouseXImpl(GLFWwindow* window) override;
	virtual float GetMouseYImpl(GLFWwindow* window) override;

};
