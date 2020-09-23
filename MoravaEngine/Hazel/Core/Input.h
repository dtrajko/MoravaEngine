#pragma once

#include "Base.h"
//	#include "KeyCodes.h"
//	#include "MouseCodes.h"


class Input
{
public:
	static bool IsKeyPressed(int key);
	static bool IsMouseButtonPressed(int button);
	static bool IsMouseButtonReleased(int button);
	static std::pair<float, float> GetMousePosition();
	static float GetMouseX();
	static float GetMouseY();
};
