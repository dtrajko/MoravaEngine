#include "WindowsWindow.h"

#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Events/ApplicationEvent.h"

#include "Log.h"

#include <cmath>


/**** BEGIN Hazel properties and methods ****/
static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description)
{
	Log::GetLogger()->error("GLFW Error ({0}) : {1}", error, description);
}
/**** END Hazel properties and methods ****/

int WindowsWindow::m_ActionPrev;

WindowsWindow::WindowsWindow()
	: WindowsWindow(1280, 720, "Window Title Undefined")
{
	xChange = 0.0f;
	yChange = 0.0f;
	xChangeReset = 0.0f;
	yChangeReset = 0.0f;
	m_CursorIgnoreLimit = 2.0f;
}

WindowsWindow::WindowsWindow(GLint windowWidth, GLint windowHeight, const char* windowTitle)
	: width(windowWidth), height(windowHeight), m_Title(windowTitle)
{
	xChange = 0.0f;
	yChange = 0.0f;
	xChangeReset = 0.0f;
	yChangeReset = 0.0f;
	m_CursorIgnoreLimit = 2.0f;

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
}

void WindowsWindow::Init(const WindowProps& props)
{
	m_Data.Title = props.Title;
	m_Data.Width = props.Width;
	m_Data.Height = props.Height;

	Log::GetLogger()->info("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

	if (!s_GLFWInitialized)
	{
		// TODO: glfwTerminate on system shutdown
		int success = glfwInit();
		Log::GetLogger()->error("Could not initialize GLFW!");
		glfwSetErrorCallback(GLFWErrorCallback);
		s_GLFWInitialized = true;
	}

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	glfwWindow = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(glfwWindow, &m_Data);
	SetVSync(true);

	// Set GLFW callbacks
	glfwSetWindowSizeCallback(glfwWindow, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;
			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

	glfwSetWindowCloseCallback(glfwWindow, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
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
		});

	glfwSetCharCallback(glfwWindow, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event);
		});

	glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow* window, int button, int action, int modes)
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
		});

	glfwSetScrollCallback(glfwWindow, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

	glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});

	glfwSetCursorEnterCallback(glfwWindow, [](GLFWwindow* window, int entered)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			// TODO
		});
}

void WindowsWindow::OnUpdate()
{
}

void WindowsWindow::SetEventCallback(const EventCallbackFn& callback)
{
}

int WindowsWindow::Initialize()
{
	// Initialize GLFW
	if (!glfwInit())
	{
		printf("GLFW initialization failed!\n");
		glfwTerminate();
		return 1;
	}

	// Setup GLFW window properties
	// OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Core profile = No backwards compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Allow forward compatibility
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	glfwWindow = glfwCreateWindow(width, height, m_Title, NULL, NULL);
	if (!glfwWindow)
	{
		printf("GLFW Window creation failed!\n");
		glfwTerminate();
		return 1;
	}

	// Get Buffer size information
	glfwGetFramebufferSize(glfwWindow, (int*)&bufferWidth, (int*)&bufferHeight);

	// Set context for GLEW to use
	glfwMakeContextCurrent(glfwWindow);

	// Handle Key and Mouse input
	CreateCallbacks();
	// SetCursorDisabled();

	// Allow modern extension features
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		printf("GLEW initialization failed!\n");
		glfwDestroyWindow(glfwWindow);
		glfwTerminate();
		return 1;
	}

	glEnable(GL_DEPTH_TEST);

	// Setup Viewport size
	glViewport(0, 0, bufferWidth, bufferHeight);

	glfwSetWindowUserPointer(glfwWindow, this);

	SetVSync(true);

	printf("GLFW and GLEW initialized.\n");

	return 0;
}

void WindowsWindow::CreateCallbacks()
{
	glfwSetKeyCallback(glfwWindow, handleKeys);
	glfwSetCursorPosCallback(glfwWindow, handleMouse);
	glfwSetMouseButtonCallback(glfwWindow, mouseButtonCallback);
	glfwSetCursorEnterCallback(glfwWindow, cursorEnterCallback);
	glfwSetWindowSizeCallback(glfwWindow, windowSizeCallback);
	glfwSetScrollCallback(glfwWindow, mouseScrollCallback);
}

void WindowsWindow::handleKeys(GLFWwindow* window, int key, int code, int action, int mode)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

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

void WindowsWindow::handleMouse(GLFWwindow* window, double xPos, double yPos)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

	theWindow->m_MouseX = (float)xPos;
	theWindow->m_MouseY = (float)yPos;

	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = (GLfloat)xPos;
		theWindow->lastY = (GLfloat)yPos;
		theWindow->mouseFirstMoved = false;
	}

	theWindow->xChange = (GLfloat)xPos - theWindow->lastX;
	theWindow->yChange = theWindow->lastY - (GLfloat)yPos;

	theWindow->xChangeReset = theWindow->xChange;
	theWindow->yChangeReset = theWindow->yChange;

	// printf("theWindow->xChange [%.2f] theWindow->yChange [%.2f]\n", theWindow->xChange, theWindow->yChange);

	theWindow->lastX = (GLfloat)xPos;
	theWindow->lastY = (GLfloat)yPos;

	// printf("x:%.2f, y:%.2f\n", theWindow->xChange, theWindow->yChange);
}

void WindowsWindow::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

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

void WindowsWindow::cursorEnterCallback(GLFWwindow* window, int entered)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

	if (entered)
		theWindow->mouseCursorAboveWindow = true;
	else
		theWindow->mouseCursorAboveWindow = false;
}

void WindowsWindow::windowSizeCallback(GLFWwindow* window, int width, int height)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

	theWindow->width = width;
	theWindow->height = height;
	glfwGetFramebufferSize(window, (int*)&theWindow->bufferWidth, (int*)&theWindow->bufferHeight);
	glViewport(0, 0, theWindow->bufferWidth, theWindow->bufferHeight);
}

void WindowsWindow::mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

	theWindow->xMouseScrollOffset = (float)xOffset;
	theWindow->yMouseScrollOffset = (float)yOffset;
}

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

void WindowsWindow::SetVSync(bool enabled)
{
	if (enabled)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(0);

	m_VSync = enabled;
}

bool WindowsWindow::IsVSync() const
{
	return m_VSync;
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
	bool isClicked = buttons[mouseButton] && !buttons_prev[mouseButton];
	buttons_prev[mouseButton] = buttons[mouseButton];
	return isClicked;
}

bool WindowsWindow::IsMouseButtonReleased(int mouseButton)
{
	bool isReleased = !buttons[mouseButton] && buttons_prev[mouseButton];
	buttons_prev[mouseButton] = buttons[mouseButton];
	return isReleased;
}

void WindowsWindow::SetShouldClose(bool shouldClose)
{
	glfwSetWindowShouldClose(glfwWindow, shouldClose);
}

void WindowsWindow::Shutdown()
{
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
}

WindowsWindow::~WindowsWindow()
{
	Shutdown();
}
