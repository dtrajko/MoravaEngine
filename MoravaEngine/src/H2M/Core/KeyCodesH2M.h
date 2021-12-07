/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <sstream>


typedef enum class KeyCodeH2M : uint16_t
{
	// From glfw3.h
	Space = 32,
	Apostrophe = 39, /* ' */
	Comma = 44, /* , */
	Minus = 45, /* - */
	Period = 46, /* . */
	Slash = 47, /* / */

	D0 = 48, /* 0 */
	D1 = 49, /* 1 */
	D2 = 50, /* 2 */
	D3 = 51, /* 3 */
	D4 = 52, /* 4 */
	D5 = 53, /* 5 */
	D6 = 54, /* 6 */
	D7 = 55, /* 7 */
	D8 = 56, /* 8 */
	D9 = 57, /* 9 */

	Semicolon = 59, /* ; */
	Equal = 61, /* = */

	A = 65,
	B = 66,
	C = 67,
	D = 68,
	E = 69,
	F = 70,
	G = 71,
	H = 72,
	I = 73,
	J = 74,
	K = 75,
	L = 76,
	M = 77,
	N = 78,
	O = 79,
	P = 80,
	Q = 81,
	R = 82,
	S = 83,
	T = 84,
	U = 85,
	V = 86,
	W = 87,
	X = 88,
	Y = 89,
	Z = 90,

	LeftBracket = 91,  /* [ */
	Backslash = 92,  /* \ */
	RightBracket = 93,  /* ] */
	GraveAccent = 96,  /* ` */

	World1 = 161, /* non-US #1 */
	World2 = 162, /* non-US #2 */

	/* Function keys */
	Escape = 256,
	Enter = 257,
	Tab = 258,
	Backspace = 259,
	Insert = 260,
	Delete = 261,
	Right = 262,
	Left = 263,
	Down = 264,
	Up = 265,
	PageUp = 266,
	PageDown = 267,
	Home = 268,
	End = 269,
	CapsLock = 280,
	ScrollLock = 281,
	NumLock = 282,
	PrintScreen = 283,
	Pause = 284,
	F1 = 290,
	F2 = 291,
	F3 = 292,
	F4 = 293,
	F5 = 294,
	F6 = 295,
	F7 = 296,
	F8 = 297,
	F9 = 298,
	F10 = 299,
	F11 = 300,
	F12 = 301,
	F13 = 302,
	F14 = 303,
	F15 = 304,
	F16 = 305,
	F17 = 306,
	F18 = 307,
	F19 = 308,
	F20 = 309,
	F21 = 310,
	F22 = 311,
	F23 = 312,
	F24 = 313,
	F25 = 314,

	/* Keypad */
	KP0 = 320,
	KP1 = 321,
	KP2 = 322,
	KP3 = 323,
	KP4 = 324,
	KP5 = 325,
	KP6 = 326,
	KP7 = 327,
	KP8 = 328,
	KP9 = 329,
	KPDecimal = 330,
	KPDivide = 331,
	KPMultiply = 332,
	KPSubtract = 333,
	KPAdd = 334,
	KPEnter = 335,
	KPEqual = 336,

	LeftShift = 340,
	LeftControl = 341,
	LeftAlt = 342,
	LeftSuper = 343,
	RightShift = 344,
	RightControl = 345,
	RightAlt = 346,
	RightSuper = 347,
	Menu = 348
} KeyH2M;

inline std::ostream& operator<<(std::ostream& os, KeyCodeH2M keyCode)
{
	os << static_cast<int32_t>(keyCode);
	return os;
}

// From glfw3.h
#define H2M_KEY_SPACE           KeyH2M::Space
#define H2M_KEY_APOSTROPHE      KeyH2M::Apostrophe    /* ' */
#define H2M_KEY_COMMA           KeyH2M::Comma         /* , */
#define H2M_KEY_MINUS           KeyH2M::Minus         /* - */
#define H2M_KEY_PERIOD          KeyH2M::Period        /* . */
#define H2M_KEY_SLASH           KeyH2M::Slash         /* / */
#define H2M_KEY_0               KeyH2M::D0
#define H2M_KEY_1               KeyH2M::D1
#define H2M_KEY_2               KeyH2M::D2
#define H2M_KEY_3               KeyH2M::D3
#define H2M_KEY_4               KeyH2M::D4
#define H2M_KEY_5               KeyH2M::D5
#define H2M_KEY_6               KeyH2M::D6
#define H2M_KEY_7               KeyH2M::D7
#define H2M_KEY_8               KeyH2M::D8
#define H2M_KEY_9               KeyH2M::D9
#define H2M_KEY_SEMICOLON       KeyH2M::Semicolon     /* ; */
#define H2M_KEY_EQUAL           KeyH2M::Equal         /* = */
#define H2M_KEY_A               KeyH2M::A
#define H2M_KEY_B               KeyH2M::B
#define H2M_KEY_C               KeyH2M::C
#define H2M_KEY_D               KeyH2M::D
#define H2M_KEY_E               KeyH2M::E
#define H2M_KEY_F               KeyH2M::F
#define H2M_KEY_G               KeyH2M::G
#define H2M_KEY_H               KeyH2M::H
#define H2M_KEY_I               KeyH2M::I
#define H2M_KEY_J               KeyH2M::J
#define H2M_KEY_K               KeyH2M::K
#define H2M_KEY_L               KeyH2M::L
#define H2M_KEY_M               KeyH2M::M
#define H2M_KEY_N               KeyH2M::N
#define H2M_KEY_O               KeyH2M::O
#define H2M_KEY_P               KeyH2M::P
#define H2M_KEY_Q               KeyH2M::Q
#define H2M_KEY_R               KeyH2M::R
#define H2M_KEY_S               KeyH2M::S
#define H2M_KEY_T               KeyH2M::T
#define H2M_KEY_U               KeyH2M::U
#define H2M_KEY_V               KeyH2M::V
#define H2M_KEY_W               KeyH2M::W
#define H2M_KEY_X               KeyH2M::X
#define H2M_KEY_Y               KeyH2M::Y
#define H2M_KEY_Z               KeyH2M::Z
#define H2M_KEY_LEFT_BRACKET    KeyH2M::LeftBracket   /* [ */
#define H2M_KEY_BACKSLASH       KeyH2M::Backslash     /* \ */
#define H2M_KEY_RIGHT_BRACKET   KeyH2M::RightBracket  /* ] */
#define H2M_KEY_GRAVE_ACCENT    KeyH2M::GraveAccent   /* ` */
#define H2M_KEY_WORLD_1         KeyH2M::World1        /* non-US #1 */
#define H2M_KEY_WORLD_2         KeyH2M::World2        /* non-US #2 */

/* Function keys */
#define H2M_KEY_ESCAPE          KeyH2M::Escape
#define H2M_KEY_ENTER           KeyH2M::Enter
#define H2M_KEY_TAB             KeyH2M::Tab
#define H2M_KEY_BACKSPACE       KeyH2M::Backspace
#define H2M_KEY_INSERT          KeyH2M::Insert
#define H2M_KEY_DELETE          KeyH2M::Delete
#define H2M_KEY_RIGHT           KeyH2M::Right
#define H2M_KEY_LEFT            KeyH2M::Left
#define H2M_KEY_DOWN            KeyH2M::Down
#define H2M_KEY_UP              KeyH2M::Up
#define H2M_KEY_PAGE_UP         KeyH2M::PageUp
#define H2M_KEY_PAGE_DOWN       KeyH2M::PageDown
#define H2M_KEY_HOME            KeyH2M::Home
#define H2M_KEY_END             KeyH2M::End
#define H2M_KEY_CAPS_LOCK       KeyH2M::CapsLock
#define H2M_KEY_SCROLL_LOCK     KeyH2M::ScrollLock
#define H2M_KEY_NUM_LOCK        KeyH2M::NumLock
#define H2M_KEY_PRINT_SCREEN    KeyH2M::PrintScreen
#define H2M_KEY_PAUSE           KeyH2M::Pause
#define H2M_KEY_F1              KeyH2M::F1
#define H2M_KEY_F2              KeyH2M::F2
#define H2M_KEY_F3              KeyH2M::F3
#define H2M_KEY_F4              KeyH2M::F4
#define H2M_KEY_F5              KeyH2M::F5
#define H2M_KEY_F6              KeyH2M::F6
#define H2M_KEY_F7              KeyH2M::F7
#define H2M_KEY_F8              KeyH2M::F8
#define H2M_KEY_F9              KeyH2M::F9
#define H2M_KEY_F10             KeyH2M::F10
#define H2M_KEY_F11             KeyH2M::F11
#define H2M_KEY_F12             KeyH2M::F12
#define H2M_KEY_F13             KeyH2M::F13
#define H2M_KEY_F14             KeyH2M::F14
#define H2M_KEY_F15             KeyH2M::F15
#define H2M_KEY_F16             KeyH2M::F16
#define H2M_KEY_F17             KeyH2M::F17
#define H2M_KEY_F18             KeyH2M::F18
#define H2M_KEY_F19             KeyH2M::F19
#define H2M_KEY_F20             KeyH2M::F20
#define H2M_KEY_F21             KeyH2M::F21
#define H2M_KEY_F22             KeyH2M::F22
#define H2M_KEY_F23             KeyH2M::F23
#define H2M_KEY_F24             KeyH2M::F24
#define H2M_KEY_F25             KeyH2M::F25

/* Keypad */
#define H2M_KEY_KP_0            KeyH2M::KP0
#define H2M_KEY_KP_1            KeyH2M::KP1
#define H2M_KEY_KP_2            KeyH2M::KP2
#define H2M_KEY_KP_3            KeyH2M::KP3
#define H2M_KEY_KP_4            KeyH2M::KP4
#define H2M_KEY_KP_5            KeyH2M::KP5
#define H2M_KEY_KP_6            KeyH2M::KP6
#define H2M_KEY_KP_7            KeyH2M::KP7
#define H2M_KEY_KP_8            KeyH2M::KP8
#define H2M_KEY_KP_9            KeyH2M::KP9
#define H2M_KEY_KP_DECIMAL      KeyH2M::KPDecimal
#define H2M_KEY_KP_DIVIDE       KeyH2M::KPDivide
#define H2M_KEY_KP_MULTIPLY     KeyH2M::KPMultiply
#define H2M_KEY_KP_SUBTRACT     KeyH2M::KPSubtract
#define H2M_KEY_KP_ADD          KeyH2M::KPAdd
#define H2M_KEY_KP_ENTER        KeyH2M::KPEnter
#define H2M_KEY_KP_EQUAL        KeyH2M::KPEqual

#define H2M_KEY_LEFT_SHIFT      KeyH2M::LeftShift
#define H2M_KEY_LEFT_CONTROL    KeyH2M::LeftControl
#define H2M_KEY_LEFT_ALT        KeyH2M::LeftAlt
#define H2M_KEY_LEFT_SUPER      KeyH2M::LeftSuper
#define H2M_KEY_RIGHT_SHIFT     KeyH2M::RightShift
#define H2M_KEY_RIGHT_CONTROL   KeyH2M::RightControl
#define H2M_KEY_RIGHT_ALT       KeyH2M::RightAlt
#define H2M_KEY_RIGHT_SUPER     KeyH2M::RightSuper
#define H2M_KEY_MENU            KeyH2M::Menu
