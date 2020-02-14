#include "Window.h"


Window::Window()
	: Window(1280, 720)
{
}

Window::Window(GLint windowWidth, GLint windowHeight)
{
	width = windowWidth;
	height = windowHeight;

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

	mainWindow = glfwCreateWindow(width, height, "Test Window", NULL, NULL);
	if (!mainWindow)
	{
		printf("GLFW Window creation failed!\n");
		glfwTerminate();
		return 1;
	}

	// Get Buffer size information
	glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);

	// Set context for GLEW to use
	glfwMakeContextCurrent(mainWindow);

	// Handle Key and Mouse input
	CreateCallbacks();
	// glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Allow modern extension features
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		printf("GLEW initialization failed!\n");
		glfwDestroyWindow(mainWindow);
		glfwTerminate();
		return 1;
	}

	glEnable(GL_DEPTH_TEST);

	// Setup Viewport size
	glViewport(0, 0, bufferWidth, bufferHeight);

	glfwSetWindowUserPointer(mainWindow, this);

	printf("GLFW and GLEW initialized.\n");

	return 0;
}

void Window::CreateCallbacks()
{
	glfwSetKeyCallback(mainWindow, handleKeys);
	glfwSetCursorPosCallback(mainWindow, handleMouse);
	glfwSetMouseButtonCallback(mainWindow, mouseButtonCallback);
	glfwSetCursorEnterCallback(mainWindow, cursorEnterCallback);
	glfwSetWindowSizeCallback(mainWindow, windowSizeCallback);
}

void Window::handleKeys(GLFWwindow* window, int key, int code, int action, int mode)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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
			// printf("Key released: %d\n", key);
		}
	}
}

void Window::handleMouse(GLFWwindow* window, double xPos, double yPos)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = (GLfloat)xPos;
		theWindow->lastY = (GLfloat)yPos;
		theWindow->mouseFirstMoved = false;
	}

	theWindow->xChange = (GLfloat)xPos - theWindow->lastX;
	theWindow->yChange = theWindow->lastY - (GLfloat)yPos;

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

	if (theWindow->buttons[GLFW_MOUSE_BUTTON_LEFT] && theWindow->mouseCursorAboveWindow)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
	glfwGetFramebufferSize(window, &theWindow->bufferWidth, &theWindow->bufferHeight);
	glViewport(0, 0, theWindow->bufferWidth, theWindow->bufferHeight);
}

GLfloat Window::getXChange()
{
	GLfloat theChange = xChange;
	xChange = 0.0f;
	return theChange;
}

GLfloat Window::getYChange()
{
	GLfloat theChange = yChange;
	yChange = 0.0f;
	return theChange;
}

Window::~Window()
{
	glfwDestroyWindow(mainWindow);
	glfwTerminate();
}
