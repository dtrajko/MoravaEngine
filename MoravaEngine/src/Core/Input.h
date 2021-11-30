#pragma once

#include "H2M/Core/KeyCodes.h"
#include "H2M/Core/MouseCodes.h"

#include <utility>


enum class CursorMode
{
	Normal = 0,
	Hidden = 1,
	Locked = 2
};

typedef enum class MouseButton : uint16_t
{
	Button0 = 0,
	Button1 = 1,
	Button2 = 2,
	Button3 = 3,
	Button4 = 4,
	Button5 = 5,
	Left = Button0,
	Right = Button1,
	Middle = Button2
} Button;

inline std::ostream& operator<<(std::ostream& os, MouseButton button)
{
	os << static_cast<int32_t>(button);
	return os;
}

class Input
{
public:
	static bool IsKeyPressed(KeyCode key);               // KeyCode key
	static bool IsMouseButtonPressed(MouseCode button);  // MouseCode button
	static bool IsMouseButtonReleased(MouseCode button); // MouseCode button
	static std::pair<float, float> GetMousePosition();
	static float GetMouseX();
	static float GetMouseY();

	static void SetCursorMode(CursorMode mode);
	static CursorMode GetCursorMode();

};
