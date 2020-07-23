#include "Window.h"

#include <cmath>


Window::Window()
	: Window(1280, 720, "Window Title Undefined")
{
	xChange = 0.0f;
	yChange = 0.0f;
	xChangeReset = 0.0f;
	yChangeReset = 0.0f;
}

Window::Window(GLint windowWidth, GLint windowHeight, const char* windowTitle)
	: width(windowWidth), height(windowHeight), m_Title(windowTitle)
{
	xChange = 0.0f;
	yChange = 0.0f;
	xChangeReset = 0.0f;
	yChangeReset = 0.0f;

	for (size_t i = 0; i < 1024; i++)
		keys[i] = false;

	for (size_t i = 0; i < 32; i++)
		buttons[i] = false;

	mouseFirstMoved = true;
	mouseCursorAboveWindow = false;
}

int Window::Initialize()
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

void Window::CreateCallbacks()
{
	glfwSetKeyCallback(glfwWindow, handleKeys);
	glfwSetCursorPosCallback(glfwWindow, handleMouse);
	glfwSetMouseButtonCallback(glfwWindow, mouseButtonCallback);
	glfwSetCursorEnterCallback(glfwWindow, cursorEnterCallback);
	glfwSetWindowSizeCallback(glfwWindow, windowSizeCallback);
	glfwSetScrollCallback(glfwWindow, mouseScrollCallback);
}

void Window::handleKeys(GLFWwindow* window, int key, int code, int action, int mode)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		// glfwSetWindowShouldClose(window, GL_TRUE);
		theWindow->SetCursorNormal();
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			theWindow->keys[key] = true;
			// printf("Key pressed: %d\n", key);
		}
		else if (action == GLFW_RELEASE)
		{
			theWindow->keys[key] = false;
		}
	}
}

void Window::handleMouse(GLFWwindow* window, double xPos, double yPos)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

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

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (button >= 0 && button < 32)
	{
		if (action == GLFW_PRESS)
		{
			theWindow->buttons[button] = true;
			// printf("Mouse button pressed: %d\n", button);
		}
		else if (action == GLFW_RELEASE)
		{
			theWindow->buttons[button] = false;
			// printf("Mouse button released: %d\n", button);
		}
	}

	if (theWindow->buttons[GLFW_MOUSE_BUTTON_MIDDLE] && theWindow->mouseCursorAboveWindow)
	{
		theWindow->SetCursorDisabled();
	}
}

void Window::cursorEnterCallback(GLFWwindow* window, int entered)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (entered)
		theWindow->mouseCursorAboveWindow = true;
	else
		theWindow->mouseCursorAboveWindow = false;
}

void Window::windowSizeCallback(GLFWwindow* window, int width, int height)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	theWindow->width = width;
	theWindow->height = height;
	glfwGetFramebufferSize(window, (int*)&theWindow->bufferWidth, (int*)&theWindow->bufferHeight);
	glViewport(0, 0, theWindow->bufferWidth, theWindow->bufferHeight);
}

void Window::mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	theWindow->xMouseScrollOffset = (float)xOffset;
	theWindow->yMouseScrollOffset = (float)yOffset;
}

GLfloat Window::getXChange()
{
	float theChange = xChange;
	return theChange;
}

GLfloat Window::getYChange()
{
	float theChange = yChange;
	return theChange;
}

float Window::getXChangeReset()
{
	float theChange = xChangeReset;
	xChangeReset = 0.0f;
	return theChange;
}

float Window::getYChangeReset()
{
	float theChange = yChangeReset;
	yChangeReset = 0.0f;
	return theChange;
}

float Window::getXMouseScrollOffset()
{
	float theOffset = xMouseScrollOffset;
	xMouseScrollOffset = 0.0f;
	return theOffset;
}

float Window::getYMouseScrollOffset()
{
	float theOffset = yMouseScrollOffset;
	yMouseScrollOffset = 0.0f;
	return theOffset;
}

void Window::SetVSync(bool enabled)
{
	if (enabled)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(0);

	m_VSync = enabled;
}

bool Window::IsVSync() const
{
	return m_VSync;
}

void Window::SetCursorDisabled()
{
	glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::SetCursorNormal()
{
	glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::SetShouldClose(bool shouldClose)
{
	glfwSetWindowShouldClose(glfwWindow, shouldClose);
}

Window::~Window()
{
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
}
