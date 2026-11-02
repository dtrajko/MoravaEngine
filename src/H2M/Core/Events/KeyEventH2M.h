/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "EventH2M.h"


namespace H2M
{

	/**
 * KeyEvent base class
 */
	class KeyEventH2M : public EventH2M
	{
	public:
		inline int GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY_H2M(EventCategoryKeyboard | EventCategoryInput)
	protected:
		KeyEventH2M(int keycode)
			: m_KeyCode(keycode) {}

		int m_KeyCode;
	};

	class KeyPressedEventH2M : public KeyEventH2M
	{
	public:
		KeyPressedEventH2M(int keycode, int repeatCount)
			: KeyEventH2M(keycode), m_RepeatCount(repeatCount) {}

		inline int GetRepeatCount() const { return m_RepeatCount; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE_H2M(KeyPressed)
	private:
		int m_RepeatCount;
	};

	class KeyReleasedEventH2M : public KeyEventH2M
	{
	public:
		KeyReleasedEventH2M(int keycode)
			: KeyEventH2M(keycode) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE_H2M(KeyReleased)
	};

	class KeyTypedEventH2M : public KeyEventH2M
	{
	public:
		KeyTypedEventH2M(int keycode)
			: KeyEventH2M(keycode) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE_H2M(KeyTyped)
	};

}
