/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/BaseH2M.h"

#include <string>
#include <sstream>
#include <functional>


namespace H2M
{

	// Events in Hazel are currently blocking, meaning when an event occurs it
	// immediately gets dispatched and must be dealt with right then an there.
	// For the future, a better strategy might be to buffer events in an event
	// bus and process them during the "event" part of the update stage.

	enum class EventTypeH2M
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	/**
	 * The purpose of EventCategory is event filtering
	 */
	enum EventCategoryH2M
	{
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4)
	};

#define EVENT_CLASS_TYPE_H2M(type) static EventTypeH2M GetStaticType() { return EventTypeH2M::type; }\
							virtual EventTypeH2M GetEventType() const override { return GetStaticType(); }\
							virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY_H2M(category) virtual int GetCategoryFlags() const override { return category; }

	/**
	 * Event - the base class for events
	 */
	class EventH2M
	{
		friend class EventDispatcherH2M;

	public:

		bool Handled = false;

		virtual EventTypeH2M GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		inline bool IsInCategory(EventCategoryH2M category)
		{
			return GetCategoryFlags() & category;
		}
	};

	/**
	 * EventDispatcher
	 */
	class EventDispatcherH2M
	{
		template<typename T>
		using EventFn = std::function<bool(T&)>;
	public:
		EventDispatcherH2M(EventH2M& event)
			: m_Event(event)
		{
		}

		// F will be deducted by the compiler
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled = func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}
	private:
		EventH2M& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const EventH2M& e)
	{
		return os << e.ToString();
	}

}
