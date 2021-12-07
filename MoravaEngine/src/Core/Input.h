#pragma once

#include "H2M/Core/KeyCodesH2M.h"
#include "H2M/Core/MouseCodesH2M.h"

#include <utility>


enum class CursorModeH2M
{
	Normal = 0,
	Hidden = 1,
	Locked = 2
};

typedef enum class MouseButtonH2M : uint16_t
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
} ButtonH2M;

inline std::ostream& operator<<(std::ostream& os, MouseButtonH2M button)
{
	os << static_cast<int32_t>(button);
	return os;
}

class Input
{
public:
	static bool IsKeyPressed(KeyCodeH2M key);               // KeyCode key
	static bool IsMouseButtonPressed(MouseCodeH2M button);  // MouseCode button
	static bool IsMouseButtonReleased(MouseCodeH2M button); // MouseCode button
	static std::pair<float, float> GetMousePosition();
	static float GetMouseX();
	static float GetMouseY();

	static void SetCursorMode(CursorModeH2M mode);
	static CursorModeH2M GetCursorMode();

};
