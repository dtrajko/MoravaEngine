/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <sstream>


typedef enum class MouseCodeH2M : uint16_t
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
} MouseH2M;

inline std::ostream& operator<<(std::ostream& os, MouseCodeH2M mouseCode)
{
	os << static_cast<int32_t>(mouseCode);
	return os;
}

#define H2M_MOUSE_BUTTON_0      MouseH2M::Button0
#define H2M_MOUSE_BUTTON_1      MouseH2M::Button1
#define H2M_MOUSE_BUTTON_2      MouseH2M::Button2
#define H2M_MOUSE_BUTTON_3      MouseH2M::Button3
#define H2M_MOUSE_BUTTON_4      MouseH2M::Button4
#define H2M_MOUSE_BUTTON_5      MouseH2M::Button5
#define H2M_MOUSE_BUTTON_6      MouseH2M::Button6
#define H2M_MOUSE_BUTTON_7      MouseH2M::Button7
#define H2M_MOUSE_BUTTON_LAST   MouseH2M::ButtonLast
#define H2M_MOUSE_BUTTON_LEFT   MouseH2M::ButtonLeft
#define H2M_MOUSE_BUTTON_RIGHT  MouseH2M::ButtonRight
#define H2M_MOUSE_BUTTON_MIDDLE MouseH2M::ButtonMiddle
