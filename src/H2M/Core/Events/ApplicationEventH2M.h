/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "EventH2M.h"


namespace H2M
{

	class WindowResizeEventH2M : public EventH2M
	{
	public:
		WindowResizeEventH2M(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {}

		inline unsigned int GetWidth() const { return m_Width; }
		inline unsigned int GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE_H2M(WindowResize);
		EVENT_CLASS_CATEGORY_H2M(EventCategoryApplication);
	private:
		unsigned int m_Width, m_Height;
	};

	class WindowCloseEventH2M : public EventH2M
	{
	public:
		WindowCloseEventH2M()
		{

		}

		EVENT_CLASS_TYPE_H2M(WindowClose);
		EVENT_CLASS_CATEGORY_H2M(EventCategoryApplication);
	};


	class AppTickEventH2M : public EventH2M
	{
	public:
		AppTickEventH2M() {}

		EVENT_CLASS_TYPE_H2M(AppTick);
		EVENT_CLASS_CATEGORY_H2M(EventCategoryApplication);
	};

	class AppUpdateEventH2M : public EventH2M
	{
	public:
		AppUpdateEventH2M() {}

		EVENT_CLASS_TYPE_H2M(AppUpdate);
		EVENT_CLASS_CATEGORY_H2M(EventCategoryApplication);
	};

	class AppRenderEventH2M : public EventH2M
	{
	public:
		AppRenderEventH2M() {}

		EVENT_CLASS_TYPE_H2M(AppRender);
		EVENT_CLASS_CATEGORY_H2M(EventCategoryApplication);
	};

}
