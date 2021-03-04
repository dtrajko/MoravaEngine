#pragma once

#include "../../Hazel/Core/KeyCodes.h"
#include "../../Hazel/Core/MouseCodes.h"

#include <utility>


class Input
{
public:
	static bool IsKeyPressed(KeyCode key);               // KeyCode key
	static bool IsMouseButtonPressed(MouseCode button);  // MouseCode button
	static bool IsMouseButtonReleased(MouseCode button); // MouseCode button
	static std::pair<float, float> GetMousePosition();
	static float GetMouseX();
	static float GetMouseY();

};
