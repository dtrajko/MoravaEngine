#pragma once

#include <sstream>


typedef enum class MouseCode : uint16_t
{
	// From glfw3.h
	Button0                = 0,
	Button1                = 1,
	Button2                = 2,
	Button3                = 3,
	Button4                = 4,
	Button5                = 5,
	Button6                = 6,
	Button7                = 7,

	ButtonLast             = Button7,
	ButtonLeft             = Button0,
	ButtonRight            = Button1,
	ButtonMiddle           = Button2
} Mouse;
	
inline std::ostream& operator<<(std::ostream& os, MouseCode mouseCode)
{
	os << static_cast<int32_t>(mouseCode);
	return os;
}

#define MORAVA_MOUSE_BUTTON_0      Mouse::Button0
#define MORAVA_MOUSE_BUTTON_1      Mouse::Button1
#define MORAVA_MOUSE_BUTTON_2      Mouse::Button2
#define MORAVA_MOUSE_BUTTON_3      Mouse::Button3
#define MORAVA_MOUSE_BUTTON_4      Mouse::Button4
#define MORAVA_MOUSE_BUTTON_5      Mouse::Button5
#define MORAVA_MOUSE_BUTTON_6      Mouse::Button6
#define MORAVA_MOUSE_BUTTON_7      Mouse::Button7
#define MORAVA_MOUSE_BUTTON_LAST   Mouse::ButtonLast
#define MORAVA_MOUSE_BUTTON_LEFT   Mouse::ButtonLeft
#define MORAVA_MOUSE_BUTTON_RIGHT  Mouse::ButtonRight
#define MORAVA_MOUSE_BUTTON_MIDDLE Mouse::ButtonMiddle
