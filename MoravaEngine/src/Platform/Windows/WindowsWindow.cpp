#include "Platform/Windows/WindowsWindow.h"

#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Events/ApplicationEvent.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include <cmath>
#include <exception>


/**** BEGIN Hazel properties and methods ****/
static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description)
{
	Log::GetLogger()->error("GLFW Error ({0}) : {1}", error, description);
}

Window* Window::Create(const WindowProps& props)
{
	return new WindowsWindow(props);
}

void WindowsWindow::SetTitle(std::string title)
{
	m_Data.Title = title;
	glfwSetWindowTitle(glfwWindow, m_Data.Title.c_str());
}

std::string WindowsWindow::GetTitle()
{
	return m_Data.Title;
}

WindowsWindow::WindowsWindow(const WindowProps& props)
{
	m_EventLoggingEnabled = false;

	Init(props);
}

WindowsWindow::~WindowsWindow()
{
	Shutdown();
}

void WindowsWindow::Init(const WindowProps& props)
{
	m_Data.Title = props.Title;
	m_Data.Width = props.Width;
	m_Data.Height = props.Height;

	xChange = 0.0f;
	yChange = 0.0f;
	xChangeReset = 0.0f;
	yChangeReset = 0.0f;
	m_CursorIgnoreLimit = 5.0f;

	for (size_t i = 0; i < 1024; i++) {
		keys[i] = false;
		keys_prev[i] = false;
	}

	for (size_t i = 0; i < 32; i++) {
		buttons[i] = false;
		buttons_prev[i] = false;
	}

	mouseFirstMoved = true;
	mouseCursorAboveWindow = false;

	Log::GetLogger()->info("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

	// Initialize GLFW
	if (!s_GLFWInitialized)
	{
		// TODO: glfwTerminate on system shutdown

		// Initialize GLFW
		if (!glfwInit())
		{
			glfwTerminate();
			throw std::runtime_error("GLFW initialization failed!");
		}

		glfwSetErrorCallback(GLFWErrorCallback);
		s_GLFWInitialized = true;

		Log::GetLogger()->info("GLFW initialized.");
	}

	// Setup GLFW window properties
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	// OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Core profile = No backwards compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Allow forward compatibility
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	glfwWindow = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
	if (!glfwWindow)
	{
		glfwTerminate();
		throw std::runtime_error("GLFW Window creation failed!");
	}

	// Set context for GLEW to use
	glfwMakeContextCurrent(glfwWindow);
	glfwSetWindowUserPointer(glfwWindow, &m_Data);
	SetVSync(true);

	// Allow modern extension features
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		glfwDestroyWindow(glfwWindow);
		glfwTerminate();
		throw std::runtime_error("GLEW initialization failed!");
	}

	Log::GetLogger()->info("GLEW initialized.");

	glEnable(GL_DEPTH_TEST);

	// Setup Viewport size
	glViewport(0, 0, m_Data.Width, m_Data.Height);

	// The old MoravaEngine method of handling events
	// not working with the new Hazel GLFW callbacks

	// SetCallbacks();
	SetCallbacksHazelDev();
}

void WindowsWindow::Shutdown()
{
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
}

void WindowsWindow::OnUpdate()
{
	// Get and handle user input events
	glfwPollEvents();

	// Swap buffers
	glfwSwapBuffers(glfwWindow);
}

void WindowsWindow::SetEventCallback(const EventCallbackFn& callback)
{
	m_Data.EventCallback = callback;
}

void WindowsWindow::SetVSync(bool enabled)
{
	if (enabled)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(0);

	m_Data.VSync = enabled;
}

bool WindowsWindow::IsVSync() const
{
	return m_Data.VSync;
}
/**** END Hazel properties and methods ****/

int WindowsWindow::m_ActionPrev;

GLfloat WindowsWindow::getXChange()
{
	float theChange = 0.0f;
	if (std::abs(xChange) > m_CursorIgnoreLimit)
		theChange = xChange;
	return theChange;
}

GLfloat WindowsWindow::getYChange()
{
	float theChange = 0.0f;
	if (std::abs(yChange) > m_CursorIgnoreLimit)
		theChange = yChange;
	return theChange;
}

float WindowsWindow::getXMouseScrollOffset()
{
	float theOffset = xMouseScrollOffset;
	xMouseScrollOffset = 0.0f;
	return theOffset;
}

float WindowsWindow::getYMouseScrollOffset()
{
	float theOffset = yMouseScrollOffset;
	yMouseScrollOffset = 0.0f;
	return theOffset;
}

void WindowsWindow::SetCursorDisabled()
{
	glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void WindowsWindow::SetCursorNormal()
{
	glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

bool WindowsWindow::IsMouseButtonClicked(int mouseButton)
{
	bool result = buttons[mouseButton] && !buttons_prev[mouseButton];
	buttons_prev[mouseButton] = buttons[mouseButton];
	return result;
}

bool WindowsWindow::IsMouseButtonReleased(int mouseButton)
{
	bool result = !buttons[mouseButton] && buttons_prev[mouseButton];
	buttons_prev[mouseButton] = buttons[mouseButton];
	return result;
}

void WindowsWindow::SetShouldClose(bool shouldClose)
{
	glfwSetWindowShouldClose(glfwWindow, shouldClose);
}

void WindowsWindow::SetInputMode(bool cursorEnabled)
{
	if (cursorEnabled)
		glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else
		glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

/**
 * Obsolete after adding GLFW callbacks to Init() method
 */
void WindowsWindow::SetCallbacks()
{
	glfwSetKeyCallback(glfwWindow, KeyCallback);
	glfwSetCharCallback(glfwWindow, CharCallback);
	glfwSetCursorPosCallback(glfwWindow, CursorPosCallback);
	glfwSetMouseButtonCallback(glfwWindow, MouseButtonCallback);
	glfwSetCursorEnterCallback(glfwWindow, CursorEnterCallback);
	glfwSetWindowSizeCallback(glfwWindow, WindowSizeCallback);
	glfwSetWindowCloseCallback(glfwWindow, WindowCloseCallback);
	glfwSetScrollCallback(glfwWindow, ScrollCallback);
}

void WindowsWindow::SetCallbacksHazelDev()
{
	// Set GLFW callbacks (Handle Key and Mouse input)
	glfwSetWindowSizeCallback(glfwWindow, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;
			WindowResizeEvent event(width, height);
			data.EventCallback(event);

			// Support for the old way of handling events
			WindowSizeCallback(window, width, height);
		});

	glfwSetWindowCloseCallback(glfwWindow, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);

			// Support for the old way of handling events
			WindowCloseCallback(window);
		});

	glfwSetKeyCallback(glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
			}

			// Support for the old way of handling events
			KeyCallback(window, key, scancode, action, mods);
		});

	glfwSetCharCallback(glfwWindow, [](GLFWwindow* window, unsigned int codepoint)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(codepoint);
			data.EventCallback(event);

			// Support for the old way of handling events
			CharCallback(window, codepoint);
		});

	glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
			}

			// Support for the old way of handling events
			MouseButtonCallback(window, button, action, mods);
		});

	glfwSetScrollCallback(glfwWindow, [](GLFWwindow* window, double xoffset, double yoffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xoffset, (float)yoffset);
			data.EventCallback(event);

			// Support for the old way of handling events
			ScrollCallback(window, xoffset, yoffset);
		});

	glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow* window, double xpos, double ypos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xpos, (float)ypos);
			data.EventCallback(event);

			// Support for the old way of handling events
			CursorPosCallback(window, xpos, ypos);
		});

	glfwSetCursorEnterCallback(glfwWindow, [](GLFWwindow* window, int entered)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			// TODO

			// Support for the old way of handling events
			CursorEnterCallback(window, entered);
		});
}

void WindowsWindow::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::KeyCallback(key {0}, scancode {1}, action {2}, mods {3})", key, scancode, action, mods);
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		// glfwSetWindowShouldClose(window, GL_TRUE);
		theWindow->SetCursorNormal();
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			theWindow->keys_prev[key] = theWindow->keys[key];
			theWindow->keys[key] = true;
			// printf("Key pressed: %d\n", key);
		}
		else if (action == GLFW_RELEASE)
		{
			theWindow->keys_prev[key] = theWindow->keys[key];
			theWindow->keys[key] = false;
		}
	}
}

void WindowsWindow::CharCallback(GLFWwindow* window, unsigned int codepoint)
{
	// not implemented/used in old MoravaEngine method of handling events
}

void WindowsWindow::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::CursorPosCallback(xpos {0}, ypos {1})", xpos, ypos);
	}

	theWindow->m_MouseX = (float)xpos;
	theWindow->m_MouseY = (float)ypos;

	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = (GLfloat)xpos;
		theWindow->lastY = (GLfloat)ypos;
		theWindow->mouseFirstMoved = false;
	}

	theWindow->xChange = (GLfloat)xpos - theWindow->lastX;
	theWindow->yChange = theWindow->lastY - (GLfloat)ypos;

	theWindow->xChangeReset = theWindow->xChange;
	theWindow->yChangeReset = theWindow->yChange;

	// printf("theWindow->xChange [%.2f] theWindow->yChange [%.2f]\n", theWindow->xChange, theWindow->yChange);

	theWindow->lastX = (GLfloat)xpos;
	theWindow->lastY = (GLfloat)ypos;

	// printf("x:%.2f, y:%.2f\n", theWindow->xChange, theWindow->yChange);
}

void WindowsWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::MouseButtonCallback(button {0}, action {1}, mods {2})", button, action, mods);
	}

	if (button >= 0 && button < 32)
	{
		if (action == GLFW_PRESS)
		{
			if (action != m_ActionPrev) {
				theWindow->buttons_prev[button] = theWindow->buttons[button];
				m_ActionPrev = action;
			}

			theWindow->buttons[button] = true;
			// printf("Mouse button pressed: %d\n", button);
		}
		else if (action == GLFW_RELEASE)
		{
			if (action != m_ActionPrev) {
				theWindow->buttons_prev[button] = theWindow->buttons[button];
				m_ActionPrev = action;
			}

			theWindow->buttons[button] = false;
			// printf("Mouse button released: %d\n", button);
		}
	}

	if (theWindow->buttons[GLFW_MOUSE_BUTTON_MIDDLE] && theWindow->mouseCursorAboveWindow)
	{
		theWindow->SetCursorDisabled();
	}
}

void WindowsWindow::CursorEnterCallback(GLFWwindow* window, int entered)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::CursorEnterCallback(entered {0})", entered);
	}

	if (entered)
		theWindow->mouseCursorAboveWindow = true;
	else
		theWindow->mouseCursorAboveWindow = false;
}

void WindowsWindow::WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::WindowSizeCallback(width {0}, height {1})", width, height);
	}

	theWindow->m_Data.Width = width;
	theWindow->m_Data.Height = height;
	glViewport(0, 0, width, height);
}

void WindowsWindow::WindowCloseCallback(GLFWwindow* window)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	// not implemented/used in old MoravaEngine method of handling events
}

void WindowsWindow::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	theWindow->xMouseScrollOffset = (float)xoffset;
	theWindow->yMouseScrollOffset = (float)yoffset;
}
