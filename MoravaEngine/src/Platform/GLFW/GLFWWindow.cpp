#include "Platform/GLFW/GLFWWindow.h"

#include "H2M/Core/Events/KeyEventH2M.h"
#include "H2M/Core/Events/MouseEventH2M.h"
#include "H2M/Core/Events/ApplicationEventH2M.h"
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#endif

#include "Core/Application.h"
#include "Core/Log.h"

#include "H2M/Renderer/RendererAPI_H2M.h"
#include "Renderer/RendererBasic.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>

GLFWWindow::GLFWWindow(const WindowSpecification& windowSpecification)
	: m_Specification(windowSpecification)
{
	m_EventLoggingEnabled = false;

	Init();
}

GLFWWindow::~GLFWWindow()
{
	Shutdown();
}

static void GLFWErrorCallback(int error, const char* description)
{
	Log::GetLogger()->error("GLFW Error ({0}) : {1}", error, description);
}

static bool s_GLFWInitialized = false;

// Linux in WSL (WSLg): windows and the mouse reach Windows through Remote Desktop, which changes what works best
static bool RunningInWSL()
{
#if defined(__linux__)
	return std::getenv("WSL_DISTRO_NAME") != nullptr;
#else
	return false;
#endif
}

void GLFWWindow::Init()
{
	m_Data.Title = m_Specification.Title;
	m_Data.Width = m_Specification.Width;
	m_Data.Height = m_Specification.Height;

	xChange = 0.0f;
	yChange = 0.0f;
	xChangeReset = 0.0f;
	yChangeReset = 0.0f;
	m_CursorIgnoreLimit = 0.0f; // no dead zone: small movements count, they are summed per frame

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

	Log::GetLogger()->info("[Window] Application working directory: {}", std::filesystem::current_path().string());
	Log::GetLogger()->info("[Window] Creating window - title: '{0}', size: [{1}x{2}]", m_Specification.Title, m_Specification.Width, m_Specification.Height);

	InitGLFW(m_Specification);

	m_RendererContext = H2M::RendererContextH2M::Create(this);
	m_RendererContext->Init();

#if defined(GRAPHICS_API_VULKAN)
	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::Vulkan)
	{
		H2M::RefH2M<H2M::VulkanContextH2M> context = m_RendererContext.As<H2M::VulkanContextH2M>();

		m_SwapChain.Init(context->GetInstance(), context->GetDevice());
		m_SwapChain.InitSurface(m_GLFW_Window);

		uint32_t width = GetWidth();
		uint32_t height = GetHeight();
		m_SwapChain.Create(&width, &height);

		H2M::VulkanAllocatorH2M::Init(context->GetDevice());
	}
#endif

	RendererBasic::SetRendererContext(m_RendererContext);

	SetVSync(true);

	RendererBasic::EnableDepthTest();
	RendererBasic::SetViewportSize(m_Data.Width, m_Data.Height);

	// The old MoravaEngine method of handling events
	// not working with the new Hazel GLFW callbacks

	// SetCallbacks();
}

void GLFWWindow::InitGLFW(const WindowSpecification& props)
{
	// Initialize GLFW
	if (!s_GLFWInitialized)
	{
		// TODO: glfwTerminate on system shutdown

#if defined(__linux__)
		// In WSL (WSLg) a Wayland window gets libdecor's frame: only a close button, edges that don't resize, and a
		// shadow that stays behind when Windows maximizes the window. An X11 window gets Weston's full frame
		// (minimize, maximize, close, resizable). Native Linux stays on GLFW's choice (Wayland on a Wayland desktop).
		if (RunningInWSL() && std::getenv("DISPLAY") && glfwPlatformSupported(GLFW_PLATFORM_X11))
		{
			glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
		}
#endif

#if defined(GRAPHICS_API_VULKAN)
		// GLFW's Vulkan functions (surface, instance extensions) through the loader the engine is linked with. On its own GLFW
		// opens libvulkan by name, only from the system's library folders: on macOS the loader is in Homebrew's or the SDK's
		glfwInitVulkanLoader(vkGetInstanceProcAddr);
#endif

		// Initialize GLFW
		if (!glfwInit())
		{
			glfwTerminate();
			std::string msg = "GLFWWindow: GLFW initialization failed!";
			Log::GetCoreLogger()->error(msg);
			throw std::runtime_error(msg);
		}

		glfwSetErrorCallback(GLFWErrorCallback);
		s_GLFWInitialized = true;

		const int platform = glfwGetPlatform();
		Log::GetLogger()->info("GLFW initialized (platform: {0}).",
			platform == GLFW_PLATFORM_WIN32 ? "Win32" : platform == GLFW_PLATFORM_COCOA ? "Cocoa" :
			platform == GLFW_PLATFORM_WAYLAND ? "Wayland" : platform == GLFW_PLATFORM_X11 ? "X11" : "other");
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

	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::Vulkan)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	}

	// Enable MSAA (multisample anti-aliasing). OpenGL has multisampling on from the start of a context.
	glfwWindowHint(GLFW_SAMPLES, 4);

	if (m_Specification.Fullscreen)
	{
		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
		m_GLFW_Window = glfwCreateWindow(mode->width, mode->height, m_Data.Title.c_str(), primaryMonitor, nullptr);
		m_GLFW_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
	}
	else
	{
		m_GLFW_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
	}

	if (!m_GLFW_Window)
	{
		glfwTerminate();
		throw std::runtime_error("GLFW Window creation failed!");
	}

	// Set context for GLEW to use
	glfwSetWindowUserPointer(m_GLFW_Window, &m_Data);

	// GLFW event callbacks
	SetCallbacksHazelDev();
}

inline std::pair<int, int> GLFWWindow::GetWindowPos() const
{
	int x, y;
	glfwGetWindowPos(m_GLFW_Window, &x, &y);
	return { x, y };
}

void GLFWWindow::ProcessEvents()
{
	// Mouse movement is summed per frame: start from zero, then the cursor callbacks add this frame's movement
	xChange = 0.0f;
	yChange = 0.0f;

	glfwPollEvents();

	//ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
	//glfwSetCursor(m_GLFW_Window, m_ImGuiMouseCursors[imgui_cursor] ? m_ImGuiMouseCursors[imgui_cursor] : m_ImGuiMouseCursors[ImGuiMouseCursor_Arrow]);
	if (!m_CursorCaptured)
	{
		glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void GLFWWindow::SwapBuffers()
{
	m_RendererContext->SwapBuffers();

	// m_SwapChain.Present();
}

void GLFWWindow::SetVSync(bool enabled)
{
	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
	{
		if (enabled) {
			glfwSwapInterval(1);
		}
		else {
			glfwSwapInterval(0);
		}
	}

	m_Data.VSync = enabled;
}

bool GLFWWindow::IsVSync() const
{
	return m_Data.VSync;
}

void GLFWWindow::Maximize()
{
	glfwMaximizeWindow(m_GLFW_Window);
}

void GLFWWindow::SetTitle(std::string title)
{
	m_Data.Title = title;
	glfwSetWindowTitle(m_GLFW_Window, m_Data.Title.c_str());
}

void GLFWWindow::SetEventCallback(const EventCallbackFn& callback)
{
	m_Data.EventCallback = callback;
}

void GLFWWindow::Shutdown()
{
	glfwDestroyWindow(m_GLFW_Window);
	glfwTerminate();
	s_GLFWInitialized = false;
}

int GLFWWindow::m_ActionPrev;

void GLFWWindow::OnUpdate()
{
	// Get and handle user input events
	// glfwPollEvents(); // TODO: move to GLFWWindow::ProcessEvents()
	ProcessEvents();

	// Swap buffers
	// glfwSwapBuffers(m_GLFW_Window); // TODO: move to GLFWWindow::SwapBuffers() / OpenGLContext::SwapBuffers()

	SwapBuffers();
}

bool GLFWWindow::IsInFocus()
{
	return glfwGetWindowAttrib(m_GLFW_Window, GLFW_FOCUSED) == GLFW_TRUE;
}

GLfloat GLFWWindow::getXChange()
{
	float theChange = 0.0f;
	if (std::abs(xChange) > m_CursorIgnoreLimit) {
		theChange = xChange;
	}
	return theChange;
}

GLfloat GLFWWindow::getYChange()
{
	float theChange = 0.0f;
	if (std::abs(yChange) > m_CursorIgnoreLimit) {
		theChange = yChange;
	}
	return theChange;
}

float GLFWWindow::getXMouseScrollOffset()
{
	float theOffset = xMouseScrollOffset;
	xMouseScrollOffset = 0.0f;
	return theOffset;
}

float GLFWWindow::getYMouseScrollOffset()
{
	float theOffset = yMouseScrollOffset;
	yMouseScrollOffset = 0.0f;
	return theOffset;
}

void GLFWWindow::SetCursorDisabled()
{
	glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GLFWWindow::SetCursorNormal()
{
	glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void GLFWWindow::SetCursorCaptured(bool captured)
{
	if (captured == m_CursorCaptured) return;
	m_CursorCaptured = captured;

	// In WSL the pointer can't be locked and its raw motion arrives distorted (too large, with vertical motion
	// that didn't happen): hide the cursor only and take the motion from its positions. The rotation then stops
	// where the hidden cursor reaches the screen edge.
	if (RunningInWSL())
	{
		glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, captured ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
		mouseFirstMoved = true;
		return;
	}

	// Disabled cursor: hidden and locked, movement keeps being reported without screen edges.
	// When it is re-enabled, GLFW puts the cursor back where it was when the capture started.
	glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

	// Raw motion bypasses the OS pointer acceleration (only used while the cursor is disabled)
	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(m_GLFW_Window, GLFW_RAW_MOUSE_MOTION, captured ? GLFW_TRUE : GLFW_FALSE);
	}

	// The cursor position jumps when the mode changes; start measuring movement again from the next position
	mouseFirstMoved = true;
}

bool GLFWWindow::GetShouldClose()
{
	return glfwWindowShouldClose(m_GLFW_Window);
}

bool GLFWWindow::IsMouseButtonClicked(int mouseButton)
{
	bool result = buttons[mouseButton] && !buttons_prev[mouseButton];
	buttons_prev[mouseButton] = buttons[mouseButton];
	return result;
}

bool GLFWWindow::IsMouseButtonReleased(int mouseButton)
{
	bool result = !buttons[mouseButton] && buttons_prev[mouseButton];
	buttons_prev[mouseButton] = buttons[mouseButton];
	return result;
}

void GLFWWindow::SetShouldClose(bool shouldClose)
{
	glfwSetWindowShouldClose(m_GLFW_Window, shouldClose);
}

#if defined(GRAPHICS_API_VULKAN)
H2M::VulkanSwapChainH2M& GLFWWindow::GetSwapChain()
{
	return m_SwapChain;
}
#endif

void GLFWWindow::SetInputMode(bool cursorEnabled)
{
	if (cursorEnabled)
		glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else
		glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

/****
 * Obsolete after adding GLFW callbacks to Init() method
 *
void GLFWWindow::SetCallbacks()
{
	glfwSetKeyCallback(m_GLFW_Window, KeyCallback);
	glfwSetCharCallback(m_GLFW_Window, CharCallback);
	glfwSetCursorPosCallback(m_GLFW_Window, CursorPosCallback);
	glfwSetMouseButtonCallback(m_GLFW_Window, MouseButtonCallback);
	glfwSetCursorEnterCallback(m_GLFW_Window, CursorEnterCallback);
	glfwSetWindowSizeCallback(m_GLFW_Window, WindowSizeCallback);
	glfwSetWindowCloseCallback(m_GLFW_Window, WindowCloseCallback);
	glfwSetScrollCallback(m_GLFW_Window, ScrollCallback);
}
****/

void GLFWWindow::SetCallbacksHazelDev()
{
	// Set GLFW callbacks (Handle Key and Mouse input)
	glfwSetWindowSizeCallback(m_GLFW_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;
			H2M::WindowResizeEventH2M event(width, height);
			data.EventCallback(event);

			// Support for the old way of handling events
			WindowSizeCallback(window, width, height);
		});

	glfwSetWindowCloseCallback(m_GLFW_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			H2M::WindowCloseEventH2M event;
			data.EventCallback(event);

			// Support for the old way of handling events
			WindowCloseCallback(window);
		});

	glfwSetKeyCallback(m_GLFW_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				H2M::KeyPressedEventH2M event(key, 0);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				H2M::KeyReleasedEventH2M event(key);
				data.EventCallback(event);
				break;
			}
			case GLFW_REPEAT:
			{
				H2M::KeyPressedEventH2M event(key, 1);
				data.EventCallback(event);
				break;
			}
			}

			// Support for the old way of handling events
			KeyCallback(window, key, scancode, action, mods);
		});

	glfwSetCharCallback(m_GLFW_Window, [](GLFWwindow* window, unsigned int codepoint)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			H2M::KeyTypedEventH2M event(codepoint);
			data.EventCallback(event);

			// Support for the old way of handling events
			CharCallback(window, codepoint);
		});

	glfwSetMouseButtonCallback(m_GLFW_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				H2M::MouseButtonPressedEventH2M event(button);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				H2M::MouseButtonReleasedEventH2M event(button);
				data.EventCallback(event);
				break;
			}
			}

			// Support for the old way of handling events
			MouseButtonCallback(window, button, action, mods);
		});

	glfwSetScrollCallback(m_GLFW_Window, [](GLFWwindow* window, double xoffset, double yoffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			H2M::MouseScrolledEventH2M event((float)xoffset, (float)yoffset);
			data.EventCallback(event);

			// Support for the old way of handling events
			ScrollCallback(window, xoffset, yoffset);
		});

	glfwSetCursorPosCallback(m_GLFW_Window, [](GLFWwindow* window, double xpos, double ypos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			H2M::MouseMovedEventH2M event((float)xpos, (float)ypos);
			data.EventCallback(event);

			// Support for the old way of handling events
			CursorPosCallback(window, xpos, ypos);
		});

	glfwSetCursorEnterCallback(m_GLFW_Window, [](GLFWwindow* window, int entered)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			// TODO

			// Support for the old way of handling events
			CursorEnterCallback(window, entered);
		});
}

void GLFWWindow::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GLFWWindow* theWindow = static_cast<GLFWWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("GLFWWindow::KeyCallback(key {0}, scancode {1}, action {2}, mods {3})", key, scancode, action, mods);
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

void GLFWWindow::CharCallback(GLFWwindow* window, unsigned int codepoint)
{
	// not implemented/used in old MoravaEngine method of handling events
}

void GLFWWindow::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	GLFWWindow* theWindow = static_cast<GLFWWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("GLFWWindow::CursorPosCallback(xpos {0}, ypos {1})", xpos, ypos);
	}

	theWindow->m_MouseX = (float)xpos;
	theWindow->m_MouseY = (float)ypos;

	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = (GLfloat)xpos;
		theWindow->lastY = (GLfloat)ypos;
		theWindow->mouseFirstMoved = false;
	}

	// Add to this frame's movement (several cursor events can arrive within one frame)
	theWindow->xChange += (GLfloat)xpos - theWindow->lastX;
	theWindow->yChange += theWindow->lastY - (GLfloat)ypos;

	theWindow->xChangeReset = theWindow->xChange;
	theWindow->yChangeReset = theWindow->yChange;

	// printf("theWindow->xChange [%.2f] theWindow->yChange [%.2f]\n", theWindow->xChange, theWindow->yChange);

	theWindow->lastX = (GLfloat)xpos;
	theWindow->lastY = (GLfloat)ypos;

	// printf("x:%.2f, y:%.2f\n", theWindow->xChange, theWindow->yChange);
}

void GLFWWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	GLFWWindow* theWindow = static_cast<GLFWWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("GLFWWindow::MouseButtonCallback(button {0}, action {1}, mods {2})", button, action, mods);
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

void GLFWWindow::CursorEnterCallback(GLFWwindow* window, int entered)
{
	GLFWWindow* theWindow = static_cast<GLFWWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("GLFWWindow::CursorEnterCallback(entered {0})", entered);
	}

	if (entered)
		theWindow->mouseCursorAboveWindow = true;
	else
		theWindow->mouseCursorAboveWindow = false;
}

void GLFWWindow::WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	GLFWWindow* theWindow = static_cast<GLFWWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("GLFWWindow::WindowSizeCallback(width {0}, height {1})", width, height);
	}

	theWindow->m_Data.Width = width;
	theWindow->m_Data.Height = height;
	glViewport(0, 0, width, height);
}

void GLFWWindow::WindowCloseCallback(GLFWwindow* window)
{
	GLFWWindow* theWindow = static_cast<GLFWWindow*>(Application::Get()->GetWindow());

	// not implemented/used in old MoravaEngine method of handling events
}

void GLFWWindow::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	GLFWWindow* theWindow = static_cast<GLFWWindow*>(Application::Get()->GetWindow());

	theWindow->xMouseScrollOffset = (float)xoffset;
	theWindow->yMouseScrollOffset = (float)yoffset;
}
