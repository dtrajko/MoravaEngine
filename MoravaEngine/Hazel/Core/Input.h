#pragma once

#include "Base.h"
#include "KeyCodes.h"
#include "MouseCodes.h"


class Input
{
public:
	static bool IsKeyPressed(KeyCode key);
	static bool IsMouseButtonPressed(MouseCode button);
	static bool IsMouseButtonReleased(MouseCode button);
	static std::pair<float, float> GetMousePosition();
	static float GetMouseX();
	static float GetMouseY();
};
